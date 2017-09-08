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

#include "QDateWrapper.h"
#include <stdlib.h>
#include <string>
#include <qlineedit.h>
#include <qvalidator.h>
#include <common/khConstants.h>

namespace qt_fusion {

static const uint32 kDateStringLength = 10;
static const uint32 kDateStringBufferLength = kDateStringLength + 1;

static const uint32 kDateTimeStringLength = 20;
static const uint32 kDateTimeStringBufferLength = kDateTimeStringLength + 1;

bool QDateWrapper::TimeIncluded() const {
  return hours_edit_ && minutes_edit_ && seconds_edit_;
}

void QDateWrapper::SetDateValidators(QObject* parent) {
  QIntValidator* year_validator = new QIntValidator(0, 9999, parent);
  QIntValidator* month_validator = new QIntValidator(0, 12, parent);
  QIntValidator* day_validator = new QIntValidator(0, 31, parent);
  year_edit_->setValidator(year_validator);
  month_edit_->setValidator(month_validator);
  day_edit_->setValidator(day_validator);

  if (TimeIncluded()) {  //  Case of raster widget.
    QIntValidator* hours_validator = new QIntValidator(0, 23, parent);
    QIntValidator* minutes_validator = new QIntValidator(0, 59, parent);
    QIntValidator* seconds_validator = new QIntValidator(0, 59, parent);
    hours_edit_->setValidator(hours_validator);
    minutes_edit_->setValidator(minutes_validator);
    seconds_edit_->setValidator(seconds_validator);
  }
}

void QDateWrapper::SetDate(uint32 year, uint32 month, uint32 day,
                           uint32 hours, uint32 minutes, uint32 seconds) {
  //  enforce some basics
  //  note we allow 0 for all 6 fields
  //  Note: no promise to validate specific dates.
  year = std::min(year, 9999U);
  month = std::min(month, 12U);
  day = std::min(day, 31U);
  hours = std::min(hours, 23U);
  minutes = std::min(minutes, 59U);
  seconds = std::min(seconds, 59U);

  char yearbuf[5];
  char datetimebuf[3];

  snprintf(yearbuf, sizeof(yearbuf), "%04d", year);
  year_edit_->setText(yearbuf);
  snprintf(datetimebuf, sizeof(datetimebuf), "%02d", month);
  month_edit_->setText(datetimebuf);
  snprintf(datetimebuf, sizeof(datetimebuf), "%02d", day);
  day_edit_->setText(datetimebuf);

  if (TimeIncluded()) {
    snprintf(datetimebuf, sizeof(datetimebuf), "%02d", hours);
    hours_edit_->setText(datetimebuf);
    snprintf(datetimebuf, sizeof(datetimebuf), "%02d", minutes);
    minutes_edit_->setText(datetimebuf);
    snprintf(datetimebuf, sizeof(datetimebuf) , "%02d", seconds);
    seconds_edit_->setText(datetimebuf);
  }
}

void QDateWrapper::SetDate(uint32 year, uint32 month, uint32 day) {
  //  enforce some basics
  //  note we allow 0 for all 3 fields
  //  mainly used for terrain and vector assets
  year = std::min(year, 9999U);
  month = std::min(month, 12U);
  day = std::min(day, 31U);  //  Note: no promise to validate specific dates.
  char buf[5];
  snprintf(buf, 5, "%04d", year);
  year_edit_->setText(buf);
  snprintf(buf, 3, "%02d", month);
  month_edit_->setText(buf);
  snprintf(buf, 3, "%02d", day);
  day_edit_->setText(buf);
}

//  Set the GUI date widget from a string of from "YYYY*MM*DDThh:mm:ss",
//  where * is any single character separator.
//  If any part of the date is invalid, we set it to 0000-00-00T00:00:00.
void QDateWrapper::SetDate(const std::string& date) {
  uint32 year = 0;
  uint32 month = 0;
  uint32 day = 0;
  uint32 hours = 0;
  uint32 minutes = 0;
  uint32 seconds = 0;

  if (date.size() >= 10) {
    std::string year_string = date.substr(0, 4);
    std::string month_string = date.substr(5, 2);
    std::string day_string = date.substr(8, 2);
    year = atoi(year_string.c_str());
    month = atoi(month_string.c_str());
    day = atoi(day_string.c_str());

    if (date.size() >= 19) {  //  For assets created with fusion 5.1.3 or higher.
      std::string hours_string = date.substr(11, 2);
      std::string minutes_string = date.substr(14, 2);
      std::string seconds_string = date.substr(17, 2);
      hours = atoi(hours_string.c_str());
      minutes = atoi(minutes_string.c_str());
      seconds = atoi(seconds_string.c_str());
      SetDate(year, month, day, hours, minutes, seconds);
    } else {
      SetDate(year, month, day);
    }
  }
}

std::string QDateWrapper::GetDate() const {
  std::string year_string = year_edit_->text().latin1();
  std::string month_string = month_edit_->text().latin1();
  std::string day_string = day_edit_->text().latin1();

  uint32 year = atoi(year_string.c_str());
  uint32 month = atoi(month_string.c_str());
  uint32 day = atoi(day_string.c_str());

  if (TimeIncluded()) {  //  Raster asset widget.
    std::string hours_string = hours_edit_->text().latin1();
    std::string minutes_string = minutes_edit_->text().latin1();
    std::string seconds_string = seconds_edit_->text().latin1();
    uint32 hours = atoi(hours_string.c_str());
    uint32 minutes = atoi(minutes_string.c_str());
    uint32 seconds = atoi(seconds_string.c_str());
    char datetimebuffer[kDateTimeStringBufferLength];
    snprintf(datetimebuffer, kDateTimeStringBufferLength,
             "%04d-%02d-%02dT%02d:%02d:%02dZ", year,
             month, day, hours, minutes, seconds);
    return std::string(datetimebuffer);
  }
    char datebuffer[kDateStringBufferLength];  //  Vector and terrain assets.
    snprintf(datebuffer, kDateStringBufferLength,
             "%04d-%02d-%02d", year, month, day);
    return std::string(datebuffer);
}

}  //  namespace qt_fusion
