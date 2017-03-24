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

//

#ifndef COMMON_GEUSERS_H__
#define COMMON_GEUSERS_H__

#include <string>
#include <sys/types.h>

// geUserId is a tuple uid, gid.
// Please follow unix concept of ruid(real user id), euid(effective user id),
// suid(saved user id) for better understanding. Further explanations my be
// found with web search on "unix real effective saved user id".
class geUserId {
 public:
  // will throw on failure
  geUserId(uid_t uid, gid_t gid);
  geUserId(const std::string &username,
           const std::string &groupname);
  const uid_t Uid() const { return uid_; }
  const gid_t Gid() const { return gid_; }

  // Following methods will throw on failure
  void SwitchEffectiveToThis() const;  // switch effective uid and gid to this
  // switch real uid and gid to this; Not reversible, if this uid is no-root
  void SwitchRealToThis() const;
  // reverse effective uid and gid to real uid and gid
  static void RestoreEffectiveUserToRealUser(void);
  // Set effective uid to new id
  static void SetEffectiveUid(const uid_t uid);
  // Set real uid to new id; This may be used by root (set-user-ID-root) programs
  // to either change to a non-root user (not reversible),
  // or restore effective user to root (reversible)
  static void SetRealUid(const uid_t uid);
  // Set effective gid to new gid
  static void SetEffectiveGid(const gid_t gid);
  // Set real gid to new gid
  static void SetRealGid(const gid_t gid);
  // GetUserIds attempts to get the user id and group id for the specified user.
  static void GetUserIds(const std::string& username, uid_t& uid, gid_t& gid);
  // GetGroupId attempts to get the group id for the specified group.
  static void GetGroupId(const std::string& groupname, gid_t& gid);

 private:
  uid_t uid_;
  gid_t gid_;
};


#endif // COMMON_GEUSERS_H__
