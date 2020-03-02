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


#include "fusion/fusionui/ServerCombinationEdit.h"

#include <qcheckbox.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qurl.h>
#include "khException.h"
#include <autoingest/.idl/storage/AssetDefs.h>
#include "fusion/fusionui/geGuiAuth.h"
#include "fusion/gepublish/PublisherClient.h"

ServerCombinationEdit::ServerCombinationEdit(QWidget* parent,
                                             const ServerCombination& c)
  : ServerCombinationEditBase(parent, 0, false, 0) {
  nickname_edit->setText(c.nickname);
  stream_url_edit->setText(c.stream.url.c_str());
  cacert_edit->setText(c.stream.cacert_ssl.c_str());
  insecure_ssl_checkbox->setChecked(c.stream.insecure_ssl);
  QueryServer();
}

ServerCombination ServerCombinationEdit::GetCombination() const {
  ServerCombination c;
  c.nickname = nickname_edit->text();
  c.stream.url = stream_url_edit->text().toUtf8().constData();
  c.stream.cacert_ssl = cacert_edit->text().toUtf8().constData();
  c.stream.insecure_ssl = insecure_ssl_checkbox->isChecked();
  return c;
}

void ServerCombinationEdit::QueryServer() {
  ServerConfig stream_server;
  stream_server.url = stream_url_edit->text().toUtf8().constData();
  stream_server.cacert_ssl = cacert_edit->text().toUtf8().constData();
  stream_server.insecure_ssl = insecure_ssl_checkbox->isChecked();

  if (stream_server.url.empty()) {
    return;
  }

  geGuiAuth auth(this);
  PublisherClient publisher_client(AssetDefs::MasterHostName(),
                                   stream_server, stream_server, NULL, &auth);

  if (!publisher_client.PingServersWithTimeout()) {
    QMessageBox::critical(
        this, "Query Failed",
        kh::tr("Error: %1\nPlease check if Server on the host '%2'"
           " is running and reachable from Fusion Host.").arg(
            publisher_client.ErrMsg().c_str(),
            stream_server.url.c_str()),
        0, 0, 0);
    query_status_label->setText("<font color=\"red\"><b>Failure</b></font>");
    return;
  }

  if (nickname_edit->text().isEmpty()) {
    nickname_edit->setText(stream_server.url.c_str());
  }
  ok_btn->setEnabled(true);
  query_status_label->setText("<font color=\"#007f00\"><b>Success</b></font>");
}

void ServerCombinationEdit::accept() {
  if (nickname_edit->text().isEmpty() ||
      stream_url_edit->text().isEmpty()) {
    QMessageBox::critical(this, "Error",
      kh::tr("Incomplete server association. The Name and URL fields are required.")
         , 0, 0, 0);
    return;
  }

  ServerCombinationEditBase::accept();
}


void ServerCombinationEdit::StreamUrlTextChanged(const QString& url_text) {
  ok_btn->setEnabled(false);
  query_status_label->setText("<none>");

  // Check whether scheme in URL is 'https' and set enabled for SSL option
  // controls.
  QUrl url(url_text);
  bool is_https = (url.protocol() == QString("https"));
  UpdateSslOptionControls(is_https);
}

void ServerCombinationEdit::CacertTextChanged() {
  ok_btn->setEnabled(false);
  query_status_label->setText("<none>");
}

void ServerCombinationEdit::InsecureSslCheckboxToggled(bool state) {
  ok_btn->setEnabled(false);
  query_status_label->setText("<none>");
}


void ServerCombinationEdit::UpdateSslOptionControls(bool enabled) {
  cacert_label->setEnabled(enabled);
  cacert_edit->setEnabled(enabled);
  insecure_ssl_checkbox->setEnabled(enabled);
  cacert_select_btn->setEnabled(enabled);
  if (!enabled) {
    cacert_edit->clear();
    insecure_ssl_checkbox->setChecked(false);
  }
}


void ServerCombinationEdit::ChooseCacertFile() {
  QString path = QFileDialog::getOpenFileName(
      cacert_edit->text(),
      kh::tr("Bundle (*.crt);;PEM format (*.pem);;All files (*.*)"),
      this,
      NULL,
      kh::tr("Select CAcert file"));

  if (path != QString::null) {
    cacert_edit->setText(path);
  }
}
