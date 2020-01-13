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
////////////////////////////////////////////////////////////////////////////////
#include <qapplication.h>
#include <string>
#include <khGetopt.h>
#include <khFileUtils.h>

#include "application.h"

void
usage(const std::string &progn, const char* msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] [<histogram>]\n"
          "   Interactive tool to generate lut files\n"
          "   Supported options are:\n"
          "      --help | -?:         Display this usage message\n"
          "      --image <image file>:  Name of initial image file to load\n"
          "      --lutwork <lutwork file>: reload previous lut work\n"
          "      --output <output>:   Name of output .lut file"
          " (default <histogram>.lut)\n",
          progn.c_str());
  exit(1);
}



#define GetNextArg() ((argn < argc) ? argv[argn++] : 0)

int main(int argc, char ** argv)
{
  try {
    // get commandline arguments
    khGetopt options;
    bool help = false;
    int argn;
    std::string lutwork;
    std::string output;
    std::string image;  // pathname of optional initial image file
    std::string progname(khBasename(argv[0]));
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt<std::string>("output", output);
    options.opt<std::string>("image", image);
    options.opt<std::string>("lutwork", lutwork);

    // validate args
    if (!options.processAll(argc, argv, argn) || help) {
      usage(progname);
    }
    const char *histogram = GetNextArg();

    // give user some feedback on chosen options
    fprintf(stderr, "\nOPTIONS SELECTED:\n");
    fprintf(stderr, "    -output  : \"%s\"\n", output.c_str());
    fprintf(stderr, "    -image   : \"%s\"\n", image.c_str());
    fprintf(stderr, "    -lutwork : \"%s\"\n", lutwork.c_str());
    fprintf(stderr, "    histogram: \"%s\"\n\n", histogram);
    fflush(stderr);

    // continue validation
    if (histogram && !khExists(histogram)) {
      usage(progname, "%s does not exist", histogram);
    }
    if (!image.empty() && !khExists(image.c_str())) {
      usage(progname, "-image file (%s) does not exist", image.c_str());
    }
    if (!lutwork.empty() && !khExists(lutwork.c_str())) {
      usage(progname, "-lutwork file (%s) does not exist", lutwork.c_str());
    }



    // Make the application window
    QApplication a( argc, argv );
    ApplicationWindow *mw = new ApplicationWindow();

    if (histogram) {
      if (output.empty()) {
        mw->setGlobalHisIn(histogram);
      } else {
        mw->setGlobalHisIn(histogram);
        mw->setGlobalLutOut(output);
      }
      char title[2048];
      snprintf(title, 2047, "%s (%s)", "Scroll", histogram);
      mw->setCaption( title );
    } else {
      if (! output.empty()) {
        usage(progname, "--output specified without a histogram file");
      }
      if (! lutwork.empty()) {
        usage(progname, "--lutwork specified without a histogram file");
      }
      mw->setCaption( "Scroll" );
    }

    mw->show();
    a.connect( &a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );

    mw->loadInitImage(image);
    if (!lutwork.empty()) mw->setLutWork(lutwork);

    return a.exec();
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;



}
