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

// TODO: High-level file comment.

#include <stdio.h>
#include <unistd.h>
#include <khFileUtils.h>
#include <notify.h>
#include <ogr_spatialref.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void usage( const char *cmd )
{
  fprintf( stderr, "usage: %s [Options] <output .prj file>\n", cmd );
  fprintf( stderr, "Options:\n" );
  fprintf( stderr, "  --esri <old esri .prj> get projection from old ESRI format \n" );
  fprintf( stderr, "  --proj4 \"proj4 string\" get projection from proj4 string\n" );
  fprintf( stderr, "  --stateplane <zone>    make state plane projection (using NAD83 datum)\n" );
  fprintf( stderr, "\n" );

  exit(1);
}


#define ARG_IS(a, b) (strcmp(a, b) == 0)

int main(int argc, char *argv[]) {
  // get comand line args
  int argn = 1;
  const char *cmdName = argv[0];
  const char *esri = 0;
  const char *proj4 = 0;
  int spzone = -1;
  const char *out = 0;
  for ( ; argn < argc; argn++ ) {
    char *arg = argv[ argn ];

    if (argv[argn][0] != '-') {
      break;
    }

    if ( ARG_IS( arg, "--esri" ) ) {
      argn++;
      if ( argn >= argc )
        usage( cmdName );
      esri = argv[argn];
    } else if ( ARG_IS( arg, "--stateplane" ) ) {
      argn++;
      if ( argn >= argc )
        usage( cmdName );
      spzone = atoi(argv[argn]);
    } else if ( ARG_IS( arg, "--proj4" ) ) {
      argn++;
      if ( argn >= argc )
        usage( cmdName );
      spzone = atoi(argv[argn]);
    }
  }

  if (argn < argc) {
    out = argv[argn++];
  }
  if (argn < argc) {
    usage(cmdName);
  }
  if (!out) {
    notify(NFY_WARN, "No output file");
    usage(cmdName);
  } else {
    fprintf(stderr, "out = %s\n", out);
  }

  OGRSpatialReference srs;

  // build from stateplane zone
  if (spzone != -1) {
    srs.SetStatePlane(spzone);
    srs.SetLinearUnits(SRS_UL_US_FOOT, atof(SRS_UL_US_FOOT_CONV));
  }

  // build from proj4 string
  if (proj4) {
  }

  // build projection from old esri .prj file
  if (esri) {
    fprintf(stderr, "doing esri\n");

    std::uint64_t size;
    time_t mtime;
    if (!khGetFileInfo(esri, size, mtime)) {
      notify(NFY_FATAL, "Unable to stat %s", esri);
    }
    char *buf = (char*)malloc(size+1);
    int fd = open(esri, O_RDONLY);
    if (fd < 0) {
      notify(NFY_FATAL, "Unable to open %s", esri);
    }
    ssize_t numread = read(fd, buf, size);
    if ((std::uint64_t)numread != size) {
      notify(NFY_FATAL, "Unable to read %s", esri);
    }
    close(fd);
    buf[size-1] = '\n';

    // break the buffer into lines
    const unsigned int maxlines = 1024;
    char *lines[maxlines+1];
    unsigned int numlines = 1;
    unsigned int pos = 0;
    lines[0] = &buf[pos];
    while ((pos < size+1) && (numlines < maxlines)) {
      if (buf[pos] == '\n') {
        buf[pos] = '\0';
        lines[numlines++] = &buf[pos+1];
      }
      ++pos;
    }
    lines[numlines] = 0;


    for (unsigned int i = 0; i < numlines; ++i) {
      if (lines[i] == 0) break;
      printf("%s\n", lines[i]);
    }


    if (srs.importFromESRI(lines) != OGRERR_NONE) {
      notify(NFY_FATAL, "Unable to interpret %s as old ESRI .prj", esri);
    }

  }

  char *wkt;
  if (srs.exportToPrettyWkt(&wkt) != OGRERR_NONE) {
    notify(NFY_FATAL, "Unable to generate well known text");
  }
  ssize_t wktlen = strlen(wkt);

  printf("----- wktlen = %lu -----\n", static_cast<unsigned long>(wktlen));
  printf("----- wkt = %s -----\n", wkt);

  khEnsureParentDir(out);
  int fd = open(out, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (fd < 0) {
    notify(NFY_FATAL, "Unable to open %s", out);
  }

  ssize_t numwritten = write(fd, wkt, wktlen);
  if (numwritten != wktlen) {
    notify(NFY_FATAL, "Unable to write %s", out);
  }
  write(fd, "\n", 1);

  OGRFree(wkt);

  close(fd);
}
