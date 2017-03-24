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


#ifndef FUSIONUI_GEGUIAUTH_H_
#define FUSIONUI_GEGUIAUTH_H_

#include <geAuth.h>
#include <pthread.h>

class AuthDialogBase;
class khMutex;
class khCondVar;
class QWidget;

class geGuiAuth : public geAuth
{
 public:
  geGuiAuth(QWidget* parent);
  ~geGuiAuth();
  bool GetUserPwd(std::string& username, std::string& password);

  // Notify the UI thread that a request is pending from another thread.
  // The UI thread can then call ExecAndSignal to pop up the modal
  // authentication dialog and signal the caller when done.
  // This assumes the UI Thread is polling this object (which is currently true
  // of the only user of this class).  If there is a need for a more efficient
  // method, that interface can be added later.
  bool IsRequestPending() const;

  // Execute the dialog and signal that we have a response.
  // The signal is needed to notify a caller from another thread that the
  // user input is available.
  void ExecAndSignal();

  // Identify this instance as running in the ui thread.
  // To instantiate the authentication dialog in another thread,
  // the geGuiAuth must be marked as NOT sychronous.
  void SetSynchronous(bool synchronous) { synchronous_ = synchronous; }

 private:
  AuthDialogBase* auth_dialog_;
  // Flag for the UI thread to test if a request is pending.
  bool dialog_request_pending_;

  // The result of the dialog: Accept or not.
  bool dialog_accept_result_;

  bool synchronous_; // Indicate that this dialog is called from the UI thread.
  // Need a condition (and mutex) to block the while waiting on the UI thread.
  khMutex* mutex_;
  khCondVar* wait_condition_;
};

#endif /* FUSIONUI_GEGUIAUTH_H_ */
