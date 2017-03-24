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


#ifndef KHSRC_FUSION_FUSIONUI_BALLOONSTYLETEXT_H__
#define KHSRC_FUSION_FUSIONUI_BALLOONSTYLETEXT_H__

#include "balloonstyletextbase.h"

class BalloonStyleText : public BalloonStyleTextBase {
 public:
  BalloonStyleText(QWidget* parent, const QString& text);

  QString GetText() const { return text_; }

  // inherited from QDialog
  virtual void accept();

  // inherited from BalloonStyleTextBase
  virtual void InsertDefault();

 private:
  QString text_;

};

#endif  // !KHSRC_FUSION_FUSIONUI_BALLOONSTYLETEXT_H__
