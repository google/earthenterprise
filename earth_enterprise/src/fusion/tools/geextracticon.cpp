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

// TODO: High-level file comment.

#include <khGetopt.h>
#include <khFileUtils.h>
#include <notify.h>
#include <iconutil/iconutil.h>

void
usage(const std::string &progn, const char *msg = 0, ...)
{
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\n"
          "usage: %s [options] --output <dst.png> <src.png>\n"
          "   Supported options are:\n"
          "      --nh         Extract normal-highlight icon pair (default)\n"
          "      --n          Extract normal icon\n"
          "      --h          Extract highlight icon\n"
          "      --l          Extract layer panel icon\n"

          ,progn.c_str() );
  exit(1);
}


int
main(int argc, char *argv[])
{
  std::string progname = argv[0];

  // process commandline options
  int argn;
  bool nh = false;
  bool n = false;
  bool h = false;
  bool l = false;
  std::string output;

  khGetopt options;
  options.flagOpt( "nh", nh );
  options.flagOpt( "n", n );
  options.flagOpt( "h", h );
  options.flagOpt( "l", l );
  options.opt( "output", output );

  if ( !options.processAll( argc, argv, argn ) ) {
    usage( progname );
  }

  // validate output filename
  if ( output.empty() )
    usage( progname, "<output> not specified" );
  if ( !khHasExtension( output, ".png" ) )
    usage( progname, "<output> must end in .png" );

  // get the source files
  khFileList filelist;
  try {
    if (argn < argc)
      filelist.AddFiles( &argv[argn], &argv[argc] );
  } catch ( const std::exception &e ) {
    usage( progname, e.what() );
  } catch (...) {
    usage( progname, "Unknown error with source files" );
  }
  if (filelist.empty()) {
    usage( progname, "Insufficient args" );
  }
  else if (!filelist[0].empty() && !khHasExtension(filelist[0], ".png") ) {
    usage( progname, "files must end in .png" );
  }


  try {

    iconutil::ExtractType extract_type = iconutil::Legend;
    if (l)  extract_type = iconutil::Legend;
    if (n)  extract_type = iconutil::Normal;
    if (h)  extract_type = iconutil::Highlight;
    if (nh) extract_type = iconutil::NormalHighlight;

    iconutil::Extract(filelist[0], extract_type, output);

  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}

