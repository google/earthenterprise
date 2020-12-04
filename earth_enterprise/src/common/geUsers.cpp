// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//

#include "geUsers.h"
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <khException.h>
#include <khGuard.h>

geUserId::geUserId(uid_t uid, gid_t gid) : uid_(uid), gid_(gid) {}

geUserId::geUserId(const std::string &username,
                   const std::string &groupname) {
  GetUserIds(username, uid_, gid_);
  if (!groupname.empty()) {
    GetGroupId(groupname, gid_);
  }
}

void geUserId::SetEffectiveUid(const uid_t uid) {
  if (seteuid(uid) == -1) {
    throw khErrnoException(kh::tr("Could not change effective user to %1")
                           .arg(uid));
  }
}
void geUserId::SetEffectiveGid(const gid_t gid) {
  if (setegid(gid) == -1) {
    throw khErrnoException(kh::tr("Could not change effective group to %1")
                           .arg(gid));
  }
}
void geUserId::SetRealUid(const uid_t uid) {
  if (setuid(uid) == -1) {
    throw khErrnoException(kh::tr("Could not change real user to %1")
                           .arg(uid));
  }
}
void geUserId::SetRealGid(const gid_t gid) {
  if (setgid(gid) == -1) {
    throw khErrnoException(kh::tr("Could not change real group to %1")
                           .arg(gid));
  }
}

void geUserId::SwitchEffectiveToThis() const {
  // set gid first. setting uid will likely remove my ability to set gid
  SetEffectiveGid(gid_);
  SetEffectiveUid(uid_);
}

void geUserId::SwitchRealToThis() const {
  // set gid first. setting uid will likely remove my ability to set gid
  SetRealGid(gid_);
  SetRealUid(uid_);
}

void geUserId::RestoreEffectiveUserToRealUser(void) {
  // The following is sequence independent but depends on saved set-user-ID.
  SetRealUid(getuid());
  SetRealGid(getgid());
}

// GetUserIds attempts to get the user id and group id for the specified user.
// It throws khErrnoException if the user is not found or encounters an error.
// Warning: this is ugly
// getpwnam_r works on goobuntu gdapper, getpwnam doesn't work,
// goobuntu ghardy has the reverse issue
void geUserId::GetUserIds(const std::string& username, uid_t& uid, gid_t& gid) {
   long bufsize = ::sysconf(_SC_GETPW_R_SIZE_MAX);
   char *buf = (char*)calloc(1, bufsize);
   khFreeGuard guard(buf);
   struct passwd pwinfo;
   struct passwd *pwinfo_ptr;
   do {
     errno = getpwnam_r(username.c_str(), &pwinfo, buf, bufsize, &pwinfo_ptr);
     if (errno != 0 && !pwinfo_ptr) {
       pwinfo_ptr = getpwnam(username.c_str());
     }
   } while (!pwinfo_ptr && (errno == EINTR));
   if (pwinfo_ptr) {
     uid = pwinfo.pw_uid;
     gid = pwinfo.pw_gid;
     return;
   }
   // Failed to get the user info.
   throw khErrnoException(kh::tr("Could not find user %1 (%d)")
                          .arg(username.c_str(), errno));
}

// GetGroupId attempts to get the group id for the specified group.
// It throws khErrnoException if the group is not found or encounters an error.
// Warning: this is ugly
// getgrnam_r doesn't work on goobuntu ghardy, getgrnam does
// goobuntu ghardy has the reverse issue
void geUserId::GetGroupId(const std::string& groupname, gid_t& gid) {
   long bufsize = ::sysconf(_SC_GETGR_R_SIZE_MAX);
   char *buf = (char*)calloc(1, bufsize);
   khFreeGuard guard(buf);
   struct group grinfo;
   struct group *grinfo_ptr;
   do {
     errno = getgrnam_r(groupname.c_str(), &grinfo, buf, bufsize,&grinfo_ptr);
     if (errno != 0 && !grinfo_ptr) {
       grinfo_ptr = getgrnam(groupname.c_str());
     }
   } while (!grinfo_ptr && (errno == EINTR));
   if (grinfo_ptr) {
     gid = grinfo_ptr->gr_gid;
     return;
   }
   // Failed to get the group info.
   throw khException(kh::tr("Could not find group %1 (%2)")
                     .arg(groupname.c_str(), errno));
}
