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

#include "LayerLegendController.h"

void LayerLegendController::SyncToConfig(void) {
  typedef LocaleMap::const_iterator LocaleIterator;

  // populate my LocaleConfig members
  sub_manager_.SyncToConfig();

  // populate my config from my LocaleConfig members
  config_->defaultLocale = default_locale_.ToLegendLocale();
  config_->locales.clear();
  for (LocaleIterator locale = other_locales_.begin();
       locale != other_locales_.end(); ++locale) {
    config_->locales[locale->first] = locale->second.ToLegendLocale();
  }
}


LayerLegendController::LayerLegendController(
    WidgetControllerManager &manager,
    LayerLegendWidget *layer_legend,
    LocaleDetails::EditMode edit_mode,
    LayerLegend *config) :
    WidgetController(manager),
    config_(config),
    default_locale_(),
    other_locales_(),
    sub_manager_(this->PopupParent())
{
  LocaleDetails::Create(sub_manager_, layer_legend,
                        edit_mode, &default_locale_, &other_locales_);

  MySyncToWidgetsImpl();

  connect(&sub_manager_, SIGNAL(widgetChanged()),
          this, SLOT(EmitChanged()));
  connect(&sub_manager_, SIGNAL(widgetTextChanged()),
          this, SLOT(EmitTextChanged()));
}


void LayerLegendController::MySyncToWidgetsImpl(void) {
  typedef LayerLegend::LocaleMap::const_iterator LocaleIterator;

  // populate my LocaleConfig members from the config
  default_locale_ = LocaleConfig::FromLegendLocale(config_->defaultLocale);
  default_locale_.ClearDefaultFlags();
  other_locales_.clear();
  for (LocaleIterator locale = config_->locales.begin();
       locale != config_->locales.end(); ++locale) {
    other_locales_[locale->first] =
      LocaleConfig::FromLegendLocale(locale->second);
  }

  // sync my LocaleConfig members to their widgets
  sub_manager_.SyncToWidgets();
}
