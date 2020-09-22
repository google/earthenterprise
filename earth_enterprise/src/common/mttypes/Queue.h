/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//

#ifndef COMMON_MTTYPES_QUEUE_H__
#define COMMON_MTTYPES_QUEUE_H__

#include "WaitBase.h"

namespace mttypes {

template <class T>
class Queue : public WaitBase {
 public:
  class Item {
   public:
    // bitmask flags
    static const int DoneFlag  = 0x1;
    static const int DrainFlag = 0x2;

    T t_;
    int  flags_;

    Item(int flags) : t_(), flags_(flags) { }
    Item(const T &t, int flags) : t_(t), flags_(flags) { }

    bool WantDrain(void) const  { return flags_ & DrainFlag; }
    bool IsDone(void) const     { return flags_ & DoneFlag; }
    void ClearDrain(void)       { flags_ &= ~DrainFlag; }
  };

  class PullHandleImpl : public khRefCounter {
   public:
    T* operator->(void) { return &t_; }

    PullHandleImpl(const T &t, Queue *backref) : backref_(backref), t_(t) { }
    virtual ~PullHandleImpl(void) {
      Release();
    }
    void Release(void) {
      if (backref_) {
        backref_->ReleasePullHandle(this);
      }
    }
    void Reuse(const T& t, Queue *backref) {
      t_ = t;
      backref_ = backref;
    }
    Queue* Backref(void) { return backref_; }
    bool HasBackref(void) const { return backref_ != 0; }
    void ClearBackref(void) { backref_ = 0; }
    T& Data(void) { return t_; }
   private:
    Queue *backref_;
    T      t_;
  };
  typedef khRefGuard<PullHandleImpl> PullHandle;


  Queue(void) : done_pushed_(false), pending_count_(0) { }
  virtual ~Queue(void) { }

  void Push(const T &t) {
    LockGuard lock(this);
    LockedPush(Item(t, 0 /* flags */));
  }
  void PushDone(void) {
    LockGuard lock(this);
    LockedPush(Item(Item::DoneFlag));
  }
  virtual void ReleaseOldAndPull(PullHandle &h) {
    LockGuard lock(this);
    if (h && h->HasBackref()) {
      LockedReleasePullHandle(&*h);
    }
    LockedPull(h, false /* want_backref */);
  }

  template <class Iterator>
  void Push(const Iterator &begin, const Iterator &end) {
    LockGuard lock(this);
    for (Iterator i = begin; i != end; ++i) {
      if (i->IsDone()) {
        done_pushed_ = true;
      }
      queue_.push_back(*i);
    }

    // Wake up anybody who may be waiting for a new item
    if (LockedOKToPull()) {
      LockedBroadcastAll();
    }
  }

  template <class Container>
  void BatchedReleaseAndPull(Container &return_queue,
                             std::uint32_t num,
                             Container &pulled_queue,
                             bool want_backref) {
    assert(num > 0);

    LockGuard lock(this);
    for (typename Container::iterator i = return_queue.begin();
         i != return_queue.end(); ++i) {
      assert((*i)->Backref() == this);
      (*i)->ClearBackref();
      --pending_count_;
    }
    if (LockedOKToPull()) {
      // wake up anybody who may be waiting for the drain
      LockedBroadcastAll();
    }

    while (1) {
      if (LockedOKToPull()) {
        // see how many are available
        std::uint32_t avail = 0;
        bool must_return = false;
        std::uint32_t tocheck = std::min(num, std::uint32_t(queue_.size()));
        for (unsigned int i = 0; i < tocheck; ++i) {
          if (queue_[i].WantDrain()) {
            avail = i;
            if ((pending_count_ == 0) && (avail == 0)) {
              avail = 1;
              must_return = true;
            } else if (avail > 0) {
              must_return = true;
            }
            break;
          }
          ++avail;
          if (queue_[i].IsDone()) {
            must_return = true;
            break;
          }
        }

        // if the number available satisfies my request
        if (must_return || (avail == num)) {
          for (unsigned int i = 0; i < avail; ++i) {
            Item &item = queue_.front();
            if (item.IsDone()) {
              // push an empty handle for Done, and don't consume it
              pulled_queue.push_back(PullHandle());
              item.ClearDrain();
            } else {
              // for all others try to reuse the PullHandle from the
              // return_queue. It will save us a bunch of new/delete's
              if (i < return_queue.size()) {
                pulled_queue.push_back(return_queue[i]);
                pulled_queue.back()->Reuse(item.t_, want_backref ? this : 0);
              } else {
                pulled_queue.push_back(
                    khRefGuardFromNew(
                        new PullHandleImpl(item.t_,
                                           want_backref ? this : 0)));
              }
              if (want_backref) {
                ++pending_count_;
              }

              // consume the item
              queue_.pop_front();
            }
          }

          // clear the return_queue so the caller can start filling it again
          return_queue.clear();
          return;
        }
      }

      // I cannot satisfy the request, block until I can
      LockedWait();
    }
  }

 protected:
  void LockedPush(const Item &item) {
    assert(!LockedIsAborted());
    assert(!done_pushed_);
    if (item.IsDone()) {
      done_pushed_ = true;
    }
    queue_.push_back(item);

    // Wake up anybody who may be waiting for a new item
    if (LockedOKToPull()) {
      if (item.IsDone()) {
        LockedBroadcastAll();
      } else {
        LockedSignalOne();
      }
    }
  }
  void LockedPull(PullHandle &h, bool want_backref) {
    assert(!LockedIsAborted());
    while (1) {
      if (LockedOKToPull()) {
        Item &item = queue_.front();
        if (item.IsDone()) {
          // NULL handle is our sentinal that we're done
          h.release();

          // Don't consume the done Item. Leave it there so all worker
          // threads will see it and exit nicely.
          item.ClearDrain();
        } else {
          if (h) {
            // reuse existing storage
            h->Reuse(item.t_, want_backref ? this : 0);
          } else {
            // nothing to reuse, make a new one
            h = khRefGuardFromNew(
                new PullHandleImpl(item.t_, want_backref ? this : 0));
          }
          if (want_backref) {
            ++pending_count_;
          }
          queue_.pop_front();
        }
        return;
      }

      LockedWait();
    }
  }
  bool LockedOKToPull(void) {
    return (!LockedIsSuspended() &&
            !queue_.empty() &&
            (!queue_.front().WantDrain() || (pending_count_ == 0)));
  }
  void LockedReleasePullHandle(PullHandleImpl *himpl) {
    assert(himpl->Backref() == this);
    assert(pending_count_ > 0);
    himpl->ClearBackref();
    --pending_count_;
    if (LockedOKToPull()) {
      // wake up anybody who may be waiting for the drain
      LockedBroadcastAll();
    }
  }

 private:
  typedef std::deque<Item> QueueImpl;
  QueueImpl queue_;
  bool      done_pushed_;
  std::uint32_t    pending_count_;

  friend class PullHandleImpl;
  void ReleasePullHandle(PullHandleImpl *himpl) {
    // Use NoThrowLockGuard since this can be called from the
    // HandleImpl's destructor. Plus, semantically it's OK to release a
    // handle after the queue has been aborted.
    NoThrowLockGuard lock(this);

    LockedReleasePullHandle(himpl);
  }

  DISALLOW_COPY_AND_ASSIGN(Queue);
};


// ****************************************************************************
// ***  WARNING: This clas is intentionally NOT MT safe
// ***
// ***  Designed to be used by a single thread to batch the pushes into
// ***  the MT safe version above
// ****************************************************************************
template <class T>
class BatchingQueuePusher {
 public:
  typedef Queue<T> QueueType;
  typedef typename Queue<T>::Item       Item;

  BatchingQueuePusher(std::uint32_t batch_size, Queue<T> &queue) :
      batch_size_(batch_size),
      queue_(queue)
  {
    assert(batch_size_ > 0);
  }

  void Flush(void) {
    if (batch_queue_.size()) {
      queue_.Push(batch_queue_.begin(), batch_queue_.end());
      batch_queue_.clear();
    }
  }

  void Push(const T &t) {
    AddToBatch(Item(t, 0 /* flags */));
  }
  void PushDone(void) {
    AddToBatch(Item(Item::DoneFlag));
    Flush();
  }

 protected:
  void AddToBatch(const Item &i) {
    batch_queue_.push_back(i);
    if (batch_queue_.size() == batch_size_) {
      Flush();
    }
  }

 private:
  std::uint32_t batch_size_;
  std::deque<Item>  batch_queue_;
  Queue<T> &queue_;
};



// ****************************************************************************
// ***  WARNING: This clas is intentionally NOT MT safe
// ***
// ***  Designed to be used by a single thread to batch the pulls from
// ***  the MT safe version above
// ****************************************************************************
template <class T>
class BatchingQueuePuller {
 public:
  typedef typename Queue<T>::PullHandle PullHandle;

  BatchingQueuePuller(std::uint32_t batch_size, Queue<T> &queue) :
      batch_size_(batch_size),
      queue_(queue),
      want_backref_(false)
  {
    assert(batch_size_ > 0);
  }

  void ReleaseOldAndPull(PullHandle &h, std::uint32_t batch_override = 0) {
    if (h && h->HasBackref()) {
      return_queue_.push_back(h);
    }
    if (!pulled_queue_.size()) {
      if (batch_override == 0) {
        batch_override = batch_size_;
      }
      queue_.BatchedReleaseAndPull(return_queue_,
                                   batch_override,
                                   pulled_queue_,
                                   want_backref_);
    }

    // if we get here, there should always be something, at least a done
    assert(pulled_queue_.size());
    h = pulled_queue_.front();
    pulled_queue_.pop_front();
  }

 private:
  std::uint32_t batch_size_;
  std::deque<PullHandle>  pulled_queue_;
  std::deque<PullHandle>  return_queue_;
  Queue<T> &queue_;

 protected:
  bool want_backref_;
};




} // namespace mttypes

#endif // COMMON_MTTYPES_QUEUE_H__
