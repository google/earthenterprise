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



#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <Qt/q3textedit.h>
#include <Qt/q3scrollview.h>

#include "AssetLog.h"

// for khstrerror
#include <notify.h>

#define BUFSZ 8192

#ifndef MIN
#define MIN(a,b) ((a<b) ? a : b)
#endif

using QTextEdit = Q3TextEdit;
using QScrollView = Q3ScrollView;
AssetLog::LogMap AssetLog::openlogs;

void
AssetLog::Open(const std::string &filename)
{
  LogMap::const_iterator found = openlogs.find(filename);
  if (found != openlogs.end()) {
    found->second->showNormal();
    found->second->raise();
  } else {
    (void)new AssetLog(filename);
  }
}


AssetLog::AssetLog( const std::string &logpath )
    : AssetLogBase( 0, NULL, false,  Qt::WDestructiveClose ), _logfile( logpath ), _logfd( -1 )
{
  _written = 0;
  logTextEdit->setUndoRedoEnabled(false);
  logTextEdit->setWordWrap(QTextEdit::WidgetWidth);
  logTextEdit->setWrapPolicy(QTextEdit::Anywhere);
  logTextEdit->setHScrollBarMode(QScrollView::AlwaysOff);
  logTextEdit->setVScrollBarMode(QScrollView::Auto);

  // start off with an initial read of the log
  timerEvent( NULL );

  // every two seconds try to read again
  startTimer( 2000 );         // interval is in milliseconds

  show();

  openlogs.insert(std::make_pair(_logfile, this));
}

AssetLog::~AssetLog()
{
  openlogs.erase(_logfile);

  if ( _logfd != -1 )
    ::close( _logfd );
}

void AssetLog::timerEvent( QTimerEvent *e )
{
  // Keep trying to open the file every time we're called.  It is possible that
  // we've been given a valid path name but the file doesn't exist yet.
  if (_logfd == -1) {
    _logfd = ::open( _logfile.c_str(), O_RDONLY );
    if (_logfd == -1)
      return;
  }

  struct stat sb;
  if ( ::fstat( _logfd, &sb ) == -1 ) {
    ::close( _logfd );
    _logfd = -1;
    return;
  }

  // never read the very last character.  it's probably a eof marker
  off_t remainder = sb.st_size - _written - 1;


  char buff[ BUFSZ ];

  // dump everything we can now
  while ( remainder ) {
    off_t n = MIN( remainder, BUFSZ - 1 );
    ssize_t sz = read( _logfd, buff, n );
    if ( sz <= 0 )
      break;
    buff[ sz ] = '\0';
    //      logTextEdit->append( buff );
    logTextEdit->insert(buff);

    remainder -= sz;
    _written += sz;
  }
}
