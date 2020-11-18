// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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
#include <Qt/qlineedit.h>
#include <Qt/qvalidator.h>
#include <Qt/qdatetime.h>
#include <common/khConstants.h>

namespace qt_fusion {

static const std::uint32_t kDateStringLength = 10;
static const std::uint32_t kDateStringBufferLength = kDateStringLength + 1;

static const std::uint32_t kDateTimeStringLength = 20;
static const std::uint32_t kDateTimeStringBufferLength = kDateTimeStringLength + 1;

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

void QDateWrapper::SetDate(std::uint32_t year, std::uint32_t month, std::uint32_t day,
                           std::uint32_t hours, std::uint32_t minutes, std::uint32_t seconds) {
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

void QDateWrapper::SetDate(std::uint32_t year, std::uint32_t month, std::uint32_t day) {
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
  std::uint32_t year = 0;
  std::uint32_t month = 0;
  std::uint32_t day = 0;
  std::uint32_t hours = 0;
  std::uint32_t minutes = 0;
  std::uint32_t seconds = 0;

  if (date.size() >= 10) {
    std::string year_string = date.substr(0, 4);
    std::string month_string = date.substr(5, 2);
    std::string day_string = date.substr(8, 2);
    year = std::stoi(year_string);
    month = std::stoi(month_string);
    day = std::stoi(day_string);

    if (date.size() >= 19) {  //  For assets created with fusion 5.1.3 or higher.
      std::string hours_string = date.substr(11, 2);
      std::string minutes_string = date.substr(14, 2);
      std::string seconds_string = date.substr(17, 2);
      hours = std::stoi(hours_string);
      minutes = std::stoi(minutes_string);
      seconds = std::stoi(seconds_string);
      SetDate(year, month, day, hours, minutes, seconds);
    } else {
      SetDate(year, month, day);
    }
  }
}

bool QDateWrapper::IsValidDate() {
  //  A date is valid if it exists or if the field is blank
  //  A value of '0000-00-00' implies the field is blank
  int zeroVal = year_edit_->text().toInt() |
                month_edit_->text().toInt() |
                day_edit_->text().toInt();
  return (zeroVal == 0) || QDate::isValid(year_edit_->text().toInt(),
                                          month_edit_->text().toInt(),
                                          day_edit_->text().toInt());
}

std::string QDateWrapper::GetDate() const {
  std::string year_string = year_edit_->text().toUtf8().constData();
  std::string month_string = month_edit_->text().toUtf8().constData();
  std::string day_string = day_edit_->text().toUtf8().constData();

  std::uint32_t year = atoi(year_string.c_str());
  std::uint32_t month = atoi(month_string.c_str());
  std::uint32_t day = atoi(day_string.c_str());

  if (TimeIncluded()) {  //  Raster asset widget.
    std::string hours_string = hours_edit_->text().toUtf8().constData();
    std::string minutes_string = minutes_edit_->text().toUtf8().constData();
    std::string seconds_string = seconds_edit_->text().toUtf8().constData();
    std::uint32_t hours = atoi(hours_string.c_str());
    std::uint32_t minutes = atoi(minutes_string.c_str());
    std::uint32_t seconds = atoi(seconds_string.c_str());
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
