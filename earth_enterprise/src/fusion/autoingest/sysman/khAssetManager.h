/*
 * Copyright 2017 Google Inc.
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


#ifndef __khAssetManager_h
#define __khAssetManager_h

#include <map>
#include <khFunctor.h>
#include <khThreadPool.h>
#include <khMTTypes.h>
#include <qdatetime.h>
#include <FusionConnection.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/.idl/AssetChanges.h>
#include <autoingest/.idl/TaskLists.h>
#include <autoingest/sysman/.idl/TaskStorage.h>
#include <autoingest/.idl/storage/DatabaseConfig.h>
#include <autoingest/.idl/storage/MapLayerConfig.h>
#include <autoingest/.idl/storage/MapProjectConfig.h>
#include <autoingest/.idl/storage/RasterProductConfig.h>
#include <autoingest/.idl/storage/RasterProjectConfig.h>
#include <autoingest/.idl/storage/VectorProductConfig.h>
#include <autoingest/.idl/storage/VectorLayerXConfig.h>
#include <autoingest/.idl/storage/VectorProjectConfig.h>
#include <autoingest/.idl/storage/KMLProjectConfig.h>
#include <autoingest/.idl/storage/MapDatabaseConfig.h>



// Later this may contain filters to only be notified in certain circumstances
class AssetListenerD {
  FusionConnection::Handle conn;
 public:
  explicit AssetListenerD(const FusionConnection::Handle &conn_) :
      conn(conn_) { }
  void SendChanges(const AssetChanges &changes) const {
    // caller handles any exceptions that may be thrown here
    // if it takes more than 4 seconds this will throw
    // and the caller will drop the connection
    conn->Notify("AssetChanges", changes, 4);
  }

  // so I can put it in a set
  bool operator< (const AssetListenerD &o) const {
    return (&*conn < &*o.conn);
  }
};


class khResourceManager;

class khAssetManagerInterface {
 public:
  virtual void NotifyVersionStateChange(const SharedString &ref,
                                        AssetDefs::State state) = 0;
  virtual void NotifyVersionProgress(const SharedString &ref, double progress) = 0;
};

class khAssetManager : public khAssetManagerInterface
{
  friend class khResourceManager;
  friend class khSystemManager;
 public:
  // redefined to avoid circular header dependencies w/ khResourceManager
  typedef khFunctor1<khResourceManager*, void> TaskCmd;

  typedef khFunctor1<khAssetManager*, void> AssetCmd;
  typedef khMTQueue<AssetCmd>               AssetCmdQueue;

  typedef khMTQueue<AssetChanges>          AssetChangesQueue;
  typedef khMTSet<AssetListenerD>          Listeners;
  typedef std::vector<AssetListenerD>      SendList;

  // routines others can call to tell me about things that need to be done
  // once the AssetGuard has been released
  virtual void NotifyVersionStateChange(const SharedString &ref,
                                        AssetDefs::State state) override;
  virtual void NotifyVersionProgress(const SharedString &ref, double progress) override;
  void SubmitTask(const SharedString &verref, const TaskDef &taskdef,
                  std::int32_t priority = 0);
  void DeleteTask(const std::string &verref);
  void DeleteFile(const std::string &path) {
    pendingFileDeletes.push_back(path);
  }

  khAssetManager(void);
  ~khAssetManager(void);
  void Init(void);
 private:
  // routines implemented in khAssetManagerDispatch.cpp
  void DispatchNotify(const FusionConnection::RecvPacket &);
  void DispatchRequest(const FusionConnection::RecvPacket &, std::string &replyPayload);

  // routines called by the Dispatch functions
  // they acquire the AssetGuard and call the supplied function
  template <class Fun>
  void DispatchThunk(Fun fun,
                     typename Fun::second_argument_type arg) {
    khLockGuard lock(mutex);
    PendingAssetGuard guard(*this);
    fun(this, arg);
  }
  template <class Fun>
  void DispatchThunk(Fun fun,
                     typename Fun::second_argument_type arg1,
                     typename Fun::third_argument_type arg2) {
    khLockGuard lock(mutex);
    PendingAssetGuard guard(*this);
    fun(this, arg1, arg2);
  }

  // ***** stuff for AssetCmdLoop *****
  AssetCmdQueue assetCmdQueue;
  void ProcessAssetCmd(const AssetCmd &) throw();
  void AssetCmdLoop(void) throw();

  // ***** stuff for AssetNotifierLoop *****
  AssetChangesQueue changesQueue;
  Listeners         listeners;
  void SendAssetChanges(const AssetChanges &changes);
  void AssetNotifierLoop(void) throw();

  // ***** stuff for the reader threads *****
  khThreadPool     *readerThreads;
  // client must not be a reference since it's type
  // is used to know what to store in the functor for the dispatch between
  // threads. But FusionConnection::Handle is made to be copied anyway.
  void HandleClientLoop(FusionConnection::Handle client) throw();
  void ClientListenerLoop(void) throw();

  // ***** asset mutex and the stuff it protects *****
  // this is no-destroy mutex because it needs to remain locked
  // even while the application exists
  khNoDestroyMutex                        mutex;
  std::map<SharedString, AssetDefs::State> pendingStateChanges;
  std::map<SharedString, double>           pendingProgress;
  std::vector<TaskCmd>                    pendingTaskCmds;
  std::vector<std::string>                pendingFileDeletes;
  void AssertPendingEmpty(void);
  void ApplyPending(void);
  void AbortPending(void) throw();

  // always sent (after pendingTaskCmds), even w/ exceptions
  std::vector<TaskCmd>                    alwaysTaskCmds;
  void SendAlwaysTaskCmds(void) throw();

  class PendingAssetGuard {
    khAssetManager &assetman;
    QTime timer;
   public:
    PendingAssetGuard(khAssetManager &assetman_);
    ~PendingAssetGuard(void);
  };
  friend class PendingAssetGuard;

  // command routines (called by AssetCmdLoop and by Dispatch routines)
  // these routines require that AssetGuard already be held
  void TaskLost(const TaskLostMsg &);
  void TaskProgress(const TaskProgressMsg &);
  void TaskDone(const TaskDoneMsg &);
  void BuildAsset(const std::string &assetref, bool &ret);
  void CancelVersion(const std::string &verref);
  void RebuildVersion(const std::string &verref);
  void SetBadVersion(const std::string &verref);
  void ClearBadVersion(const std::string &verref);
  void CleanVersion(const std::string &verref);
  void RasterProductImport(const RasterProductImportRequest &req);
  void MercatorRasterProductImport(const RasterProductImportRequest &req);
  void RasterProjectEdit(const RasterProjectEditRequest &req);
  void RasterProjectNew(const RasterProjectModifyRequest &req);
  void RasterProjectModify(const RasterProjectModifyRequest &req);
  void RasterProjectAddTo(const RasterProjectModifyRequest &req);
  void RasterProjectDropFrom(const RasterProjectDropFromRequest &req);
  void MercatorRasterProjectEdit(const RasterProjectEditRequest &req);
  void MercatorRasterProjectNew(const RasterProjectModifyRequest &req);
  void MercatorRasterProjectModify(const RasterProjectModifyRequest &req);
  void MercatorRasterProjectAddTo(const RasterProjectModifyRequest &req);
  void MercatorRasterProjectDropFrom(const RasterProjectDropFromRequest &req);
  void VectorProductImport(const VectorProductImportRequest &req);
  void VectorLayerXEdit(const VectorLayerXEditRequest &req);
  void VectorProjectEdit(const VectorProjectEditRequest &req);
  void VectorProjectNew(const VectorProjectNewRequest &req);
  void VectorProjectModify(const VectorProjectModifyRequest &req);
  void VectorProjectAddTo(const VectorProjectModifyRequest &req);
  void VectorProjectDropFrom(const VectorProjectDropFromRequest &req);
  void DatabaseEdit(const DatabaseEditRequest &req);
  void DatabaseNew(const DatabaseEditRequest &req);
  void DatabaseModify(const DatabaseEditRequest &req);
  void GetCurrTasks(const std::string &dummy, TaskLists &ret);
  void ReloadConfig(const std::string &dummy);
  void MapProjectEdit(const MapProjectEditRequest &req);
  void MapLayerEdit(const MapLayerEditRequest &req);
  void KMLProjectEdit(const KMLProjectEditRequest &req);
  void MapDatabaseEdit(const MapDatabaseEditRequest &req);
  void MapDatabaseNew(const MapDatabaseEditRequest &req);
  void MapDatabaseModify(const MapDatabaseEditRequest &req);
  void MercatorMapDatabaseEdit(const MapDatabaseEditRequest &req);
  void MercatorMapDatabaseNew(const MapDatabaseEditRequest &req);
  void MercatorMapDatabaseModify(const MapDatabaseEditRequest &req);
  // Re-import source files if updated.
  void ProductReImport(const std::string &assetref, bool &updated);

  void MakeAssetDir(const std::string &assetdir);

  // Protobuf-based methods
  static const std::string ASSET_STATUS;
  static const std::string PUSH_DATABASE;
  static const std::string PUBLISH_DATABASE;
  static const std::string GET_TASKS;

 /**
  * Get build status of each version of an asset and return as a
  * serialized string.
  * @return serialized status of asset versions.
  */
  // TODO: return results as serialized protobuf.
  std::string GetAssetStatus(const std::string asset_name);

 /**
  * Push database according to serialized arguments.
  * @param arguments_string Comma-separated hostname, database name.
  * E.g. bplinux,Databases/CA.kdatabase?version=43
  * @return serialized return values.
  */
  // TODO: receive and return results as serialized protobufs.
  std::string PushDatabase(const std::string arguments_string);

 /**
  * Publish database according to serialized arguments.
  * @param arguments_string Comma-separated hostname, database name,
  *                         stream server, search server.
  * E.g. bplinux,Databases/CA.kdatabase?version=43,default_ge,default_search
  * @return serialized return values.
  */
  // TODO: receive and return results as serialized protobufs.
  std::string PublishDatabase(const std::string arguments_string);

  /**
   * Retrieve the current System Manager tasking
   * @param msg Received request message from client
   * @return serialized tasks list or error message beginning with "ERROR:"
   */
  std::string RetrieveTasking(const FusionConnection::RecvPacket& msg);
};

extern khAssetManager theAssetManager;

#endif /* __khAssetManager_h */
