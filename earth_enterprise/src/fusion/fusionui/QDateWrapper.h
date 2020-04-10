/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

#ifndef FUSION_FUSIONUI_QTDATEWRAPPER_H__
#define FUSION_FUSIONUI_QTDATEWRAPPER_H__

#include <common/khTypes.h>
#include <string>

class QObject;
class QLineEdit;

namespace qt_fusion {

// QDateWrapper wraps 6 QLineEdit (QT UI toolkit) Controls to manage a
// more flexible date input widget than the QDateEdit control.
// This allows us to accept dates such as:
// "0000-00-00" undefined
// "2001-01-00" day unknown
// "2001-00-00" day/month unknown
// Note: no checking of the validity of the actual day is performed...i.e.,
// "2001-02-30" will be accepted.
// In addition version 5.1.3 onwards, dates with hh:mm:ss granularity would be
// accepted for Imagery assets. That allows us to accept dates such as:
// "2016-12-31:23:59:59" valid datetime.
// "0000-00-00T00:00:00" undefined date and time.
// "2016-12-31:00:00:00  day known, time unknown.

class QDateWrapper {
public:
 // Initialize the parent dialog and wrapper with the 6 line edit controls.
 // The wrapper takes no ownership of the controls.
 QDateWrapper(QObject* parent_dialog, QLineEdit* year, QLineEdit* month,
              QLineEdit* day, QLineEdit* hours, QLineEdit* minutes,
              QLineEdit* seconds)
     : year_edit_(year),
       month_edit_(month),
       day_edit_(day),
       hours_edit_(hours),
       minutes_edit_(minutes),
       seconds_edit_(seconds) {
   SetDateValidators(parent_dialog);
 }

 QDateWrapper(QObject* parent_dialog, QLineEdit* year, QLineEdit* month,
              QLineEdit* day)
     : year_edit_(year),
       month_edit_(month),
       day_edit_(day),
       hours_edit_(NULL),
       minutes_edit_(NULL),
       seconds_edit_(NULL) {
   SetDateValidators(parent_dialog);
  }
  ~QDateWrapper() {}

  // Set the Date GUI text boxes given the integral date values.
  // Note: 0 is valid for all 6 fields.
  // This routine enforces that year <= 9999, month <= 12 and day <= 31
  // for time it enforces that hours <=23 and minutes <=59 and seconds <=59.
  // without throwing an exception. i.e., year 10000 will appear as 9999.
  // no checking for date validity (i.e., 2008-02-30) is performed.
  void SetDate(std::uint32_t year, std::uint32_t month, std::uint32_t day, std::uint32_t hours,
               std::uint32_t minutes, std::uint32_t seconds);

  // For Vector asset resources as they don't support time granularity.
  void SetDate(std::uint32_t year, std::uint32_t month, std::uint32_t day);

  //  Set the GUI date widget from a string of from "YYYY*MM*DDThh*mm*ss"
  //  or "YYYY*MM*DD" for vector assets.
  //  where * is any single character separator.
  //  If any part of the date is invalid, we set it to 0000-00-00 00:00:00
  //  or 0000-00-00 whichever is applicable.
  void SetDate(const std::string& date);

  // Get the string value of the date from the GUI date widget.
  // Will be of form "YYYY-MM-DDThh:mm:ssZ" for Imagery assets and
  // "YYYY-MM-DD" for Terrain and vector assets.
  std::string GetDate() const;

  // Check if the date occurs before the current system date.
  bool IsValidDate();

private:
  // Date utilities
  // Set some basic date validators for the line edit widgets and make sure
  // to associate them to the parent dialog.
  void SetDateValidators(QObject* parent);

  // Indicates whether this date has a valid time component
  bool TimeIncluded() const;

  QLineEdit* year_edit_;
  QLineEdit* month_edit_;
  QLineEdit* day_edit_;
  QLineEdit* hours_edit_;
  QLineEdit* minutes_edit_;
  QLineEdit* seconds_edit_;
};

}

#endif  // FUSION_FUSIONUI_QTDATEWRAPPER_H__
