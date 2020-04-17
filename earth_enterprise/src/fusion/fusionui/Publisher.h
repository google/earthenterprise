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


#ifndef KHSRC_FUSION_FUSIONUI_PUBLISHER_H__
#define KHSRC_FUSION_FUSIONUI_PUBLISHER_H__

#include <publisherbase.h>
#include <autoingest/.idl/ServerCombination.h>

class Publisher : public PublisherBase {
 public:
  Publisher(QWidget* parent, bool modal, Qt::WFlags flags);

  // inherited from QDialog
  virtual void accept();

  // inherited from PublisherBase
  virtual void AddCombination();
  virtual void EditCombination();
  virtual void DeleteCombination();
  virtual void MoveUp();
  virtual void MoveDown();
  virtual void CurrentChanged(int row, int col);

 private:
  ServerCombination GetCombinationFromRow(int row);
  ServerCombination EditCombination(const ServerCombination& current_combination);
  void SetRow(int row, const ServerCombination& c);
  void SwapRows(int a, int b);

  ServerCombinationSet server_combination_set_;
};

#endif  // !KHSRC_FUSION_FUSIONUI_PUBLISHER_H__
