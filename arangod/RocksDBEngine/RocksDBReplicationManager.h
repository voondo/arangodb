////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2017 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Simon Grätzer
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGO_ROCKSDB_ROCKSDB_REPLICATION_MANAGER_H
#define ARANGO_ROCKSDB_ROCKSDB_REPLICATION_MANAGER_H 1

#include "Basics/Common.h"
#include "Basics/Mutex.h"
#include "RocksDBEngine/RocksDBReplicationContext.h"

struct TRI_vocbase_t;

namespace arangodb {

typedef uint64_t RocksDBReplicationId;

class RocksDBReplicationManager {
 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief create a contexts repository
  //////////////////////////////////////////////////////////////////////////////

  explicit RocksDBReplicationManager();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief destroy a contexts repository
  //////////////////////////////////////////////////////////////////////////////

  ~RocksDBReplicationManager();

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief creates a new context which must be later returned using release()
  /// or destroy(); guarantees that RocksDB file deletion is disabled while
  /// there are active contexts
  //////////////////////////////////////////////////////////////////////////////

  RocksDBReplicationContext* createContext(double ttl);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief remove a context by id
  //////////////////////////////////////////////////////////////////////////////

  bool remove(RocksDBReplicationId);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief find an existing context by id
  /// if found, the context will be returned with the isUsed() flag set to true.
  /// it must be returned later using release() or destroy()
  /// the second parameter shows if the context you are looking for is busy or
  /// not
  //////////////////////////////////////////////////////////////////////////////

  RocksDBReplicationContext* find(
      RocksDBReplicationId, bool& isBusy,
      double ttl = RocksDBReplicationContext::DefaultTTL);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return a context for later use
  //////////////////////////////////////////////////////////////////////////////

  void release(RocksDBReplicationContext*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return a context for garbage collection
  //////////////////////////////////////////////////////////////////////////////

  void destroy(RocksDBReplicationContext*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the repository contains a used context
  //////////////////////////////////////////////////////////////////////////////

  bool containsUsedContext();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief drop contexts by database (at least mark them as deleted)
  //////////////////////////////////////////////////////////////////////////////

  void drop(TRI_vocbase_t*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief drop all contexts (at least mark them as deleted)
  //////////////////////////////////////////////////////////////////////////////

  void dropAll();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief run a garbage collection on the contexts
  //////////////////////////////////////////////////////////////////////////////

  bool garbageCollect(bool);
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief tell the replication manager that a shutdown is in progress
  /// effectively this will block the creation of new contexts
  //////////////////////////////////////////////////////////////////////////////
    
  void beginShutdown();

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief mutex for the contexts repository
  //////////////////////////////////////////////////////////////////////////////

  Mutex _lock;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief list of current contexts
  //////////////////////////////////////////////////////////////////////////////

  std::unordered_map<RocksDBReplicationId, RocksDBReplicationContext*>
      _contexts;
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not a shutdown is in progress
  //////////////////////////////////////////////////////////////////////////////

  bool _isShuttingDown;
};

class RocksDBReplicationContextGuard {
 public:
  
  RocksDBReplicationContextGuard(RocksDBReplicationManager* manager,
                                 RocksDBReplicationContext* ctx)
    : _manager(manager), _ctx(ctx) {}
  
  ~RocksDBReplicationContextGuard()  {
    if (_ctx != nullptr) {
      _manager->release(_ctx);
    }
  }

 private:
  RocksDBReplicationManager* _manager;
  RocksDBReplicationContext* _ctx;
};
}

#endif
