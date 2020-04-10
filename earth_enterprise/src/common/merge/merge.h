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


#ifndef COMMON_MERGE_MERGE_H__
#define COMMON_MERGE_MERGE_H__

// Merge
//
// Classes for multi-way merge

#include <string>
#include <vector>
#include <queue>
#include <common/base/macros.h>
#include <khSimpleException.h>
#include <khGuard.h>

// MergeSource class - abstract class template, subclassed for
// different types of sources.  The "name" parameter is used for
// debugging and error reporting.
//
// The instantiable subclass must define operators for Advance(),
// Current(), and Close(), as well as constructor and destructor.  The
// subclass must also provide a ">" (follows) operator.
//
// Current() always presents the current value of the source
// (generates an exception if past end).  The first value must be
// available on exit from the constructor.  Advance() advances the
// source to the next value, returning true for success and false if
// no more values are available.  A value returned by Current() is not
// valid after the next Advance().

template <class ValueType> class MergeSource {
 public:
  typedef ValueType MergeType;
  MergeSource(const std::string &name) : name_(name) {}
  virtual ~MergeSource() {}
  virtual const ValueType &Current() const = 0;
  virtual bool Advance() = 0;           // to next position (false = end)
  virtual void Close() = 0;
  const std::string &name() const { return name_; }
 private:
  const std::string name_;
  DISALLOW_COPY_AND_ASSIGN(MergeSource<ValueType>);
};

// Main multi-way merge class.  To use, instantiate a Merge with some
// ValueType, add some sources, and Start.  Use Current() to access
// the current ValueType item, and Advance() to process the next merge
// item.  Use Close() when done.
//
// Note on multi-threading: because Merge is inherently serial, it
// doesn't make sense to access a Merge from multiple threads.  The
// MergeSource objects can, of course, be driven by multiple threads.
// A multi-threaded MergeSource implementation is responsible for its
// own thread coordinatation.

template <class ValueType> class Merge : public MergeSource<ValueType> {
 private:
  // QueueItem - private internal class to encapsulate priority_queue
  // items.  The priority_queue and underlying heap algorithms do
  // comparisons on the items in the priority queue.  Since we want to
  // store pointers in the queue, we need to encapsulate the pointers
  // so that the comparisons are done on the underlying MergeSource
  // objects.  Note that copying must be allowed for the STL
  // priority_queue to function properly.
  class QueueItem {
   public:
    QueueItem(const khTransferGuard<MergeSource<ValueType> > &item,
              unsigned int id) :
        item_(item.take()),
        id_(id)
    {}
    inline unsigned int Id(void) const { return id_; }
    inline bool operator>(const QueueItem &other) const {
      return item_->Current() > other.item_->Current();
    }
    inline MergeSource<ValueType>& operator*() const { return *item_; }
    inline MergeSource<ValueType>* operator->() const { return item_; }

    // Delete() is done through an explicit method rather than a
    // destructor in order to allow copying of QueueItem.
    void Delete() {
      delete item_;
      item_ = NULL;
    }
   private:
    MergeSource<ValueType> *item_;
    unsigned int id_;
  };

 public:
  Merge(const std::string &name)
      : MergeSource<ValueType>(name),
        started_(false),
        next_source_id_(0)
  {
  }

  virtual ~Merge() {
    Close();
  }

  virtual void Close() {
    if (!std::uncaught_exception()) {
      if (!sources_.empty()) {
        notify(NFY_WARN, "Merge::Close: %llu sources not empty in: %s",
               static_cast<long long unsigned>(sources_.size()),
               MergeSource<ValueType>::name().c_str());
      }
    }

    // Ensure that any MergeSource items left in queue are deleted.
    while (!sources_.empty()) {
      QueueItem source = sources_.top();
      sources_.pop();
      source.Delete();
    }
  }

  bool Finished() const { return started_  &&  sources_.empty(); }
  bool Active() const { return started_  &&  !sources_.empty(); }

  // Get name of current source at top of queue (for debug).  Returns
  // empty string if no sources.
  const std::string &CurrentName() {
    static const std::string empty;
    if (!sources_.empty()) {
      return sources_.top()->name();
    } else {
      return empty;
    }
  }

  // Add a source to the merge.  If an add is done after Start(),
  // the first item from the new source must be strictly > Current().
  void AddSource(const khTransferGuard<MergeSource<ValueType> > &source) {
    if (!started_  ||  (!sources_.empty() && source->Current() > Current())) {
      sources_.push(QueueItem(source, next_source_id_++));
    } else {
      throw khSimpleException("Merge::AddSource: "
                              "attempted on open merge out of order: ")
        << MergeSource<ValueType>::name();
    }
  }

  // Start the merge
  void Start() {                        // start the merge
    if (!started_) {
      if (!sources_.empty()) {
        started_ = true;
      } else {
        throw khSimpleException("Merge::Start: no sources for Merge ")
          << MergeSource<ValueType>::name();
      }
    } else {
      throw khSimpleException("Merge::Start: already started: ")
        << MergeSource<ValueType>::name();
    }
  }

  virtual const ValueType &Current() const {  // first source on heap
    if (Active()) {
      return sources_.top()->Current();
    } else {
      throw khSimpleException("Merge::Current: not started or no open sources: ")
        << MergeSource<ValueType>::name();
    }
  }

  const unsigned int CurrentSourceId() const {  // first source on heap
    if (Active()) {
      return sources_.top().Id();
    } else {
      throw khSimpleException("Merge::CurrentSourceId: not started or no open sources: ")
        << MergeSource<ValueType>::name();
    }
  }

  // Advance first source and reorder
  virtual bool Advance() {
    if (Active()) {
      QueueItem top = sources_.top();
      sources_.pop();
      if (top->Advance()) {               // if more
        sources_.push(top);
      } else {                            // else end of data
        top->Close();
        top.Delete();
      }
      return !sources_.empty();
    } else {
      throw khSimpleException("Merge::Advance: not started or no open sources: ")
        << MergeSource<ValueType>::name();
    }
  }

 private:
  bool started_;                        // true after Start called
  unsigned int next_source_id_;

  // Min-heap priority queue of sources.  Close() or ~Merge() will
  // delete any sources left over in the queue.
  std::priority_queue<QueueItem,
                      std::vector<QueueItem>,
                      std::greater<QueueItem> > sources_;
  DISALLOW_COPY_AND_ASSIGN(Merge);
};

#endif  // COMMON_MERGE_MERGE_H__
