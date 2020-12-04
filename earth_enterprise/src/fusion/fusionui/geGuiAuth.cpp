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


#include "geGuiAuth.h"
#include "authdialogbase.h"
#include <qlineedit.h>
#include "khThread.h"

geGuiAuth::geGuiAuth(QWidget* parent) {
  const char* name = NULL;
  auth_dialog_ = new AuthDialogBase(parent, name, false, 0);
  dialog_request_pending_ = false;
  dialog_accept_result_ = false;

  // This instance is assumed to be running in the UI Thread by default.
  synchronous_ = true;
  wait_condition_ = new khCondVar();
  mutex_ = new khMutex();
}

geGuiAuth::~geGuiAuth() {
  if (auth_dialog_)
    delete auth_dialog_;
  if (wait_condition_)
    delete wait_condition_;
  if (mutex_)
    delete mutex_;
}

// Open the auth dialog and wake the process waiting on the dialog
// results.
void geGuiAuth::ExecAndSignal()
{
  khLockGuard locker(*mutex_);
  dialog_request_pending_ = false;
  // Send the calling thread an event.

  // Open the dialog and save the result.
  dialog_accept_result_ = (auth_dialog_->exec() == QDialog::Accepted);

  // Unblock the calling thread if asynchronously called.
  wait_condition_->signal_one();
}

// Notify the UI thread that a request is pending from another thread.
bool geGuiAuth::IsRequestPending() const
{
  // This mutex is required to prevent a race condition.
  khLockGuard locker(*mutex_);
  return dialog_request_pending_;
}

// Get the username and password from the Fusion user.
// This pops up a dialog in the UI thread.
// If "synchronous" is not correctly set, this will likely crash.
bool geGuiAuth::GetUserPwd(std::string& username, std::string& password) {
  khLockGuard locker(*mutex_);
  // Wait until the dialog is set.
  if (synchronous_) {
    // Synchronous execution in this thread.
    dialog_accept_result_ = (auth_dialog_->exec() == QDialog::Accepted);
  } else {
    // Asynchronous execution in the UI Thread.
    // Notify the UI thread that we need a user response.
    dialog_request_pending_ = true;
    // Asynchronous call must wait for completion.
    wait_condition_->wait(*mutex_);
  }

  // If the user pressed Cancel, don't modify the username or password.
  if (dialog_accept_result_) {
    username = auth_dialog_->username_edit->text().toUtf8().constData();
    password = auth_dialog_->password_edit->text().toUtf8().constData();
  }
  return dialog_accept_result_;
}

