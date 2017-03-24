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


#include <notify.h>
#include <stdlib.h>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <string>

#include "common/khAbortedException.h"
#include "common/khFileUtils.h"
#include "common/khGetopt.h"
#include "fusion/portableglobe/polygontoqtnodes.h"

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s \\\n"
          "           --kml_polygon_file=<file_name_string> \\\n"
          "           --qt_nodes_file=<file_name_string> \\\n"
          "           --max_level=<max_level_int>\n"
          "\n"
          " Creates a list of quadtree nodes that encompass a polygon at the\n"
          " given max_level.\n"
          "\n"
          " Options:\n"
          "   --kml_polygon_file: KML file containing a polygon that defines\n"
          "                       the region of interest.\n"
          "   --qt_nodes_file: File where quadtree addresses are stored.\n"
          "   --max_level: Level of resolution of quadtree that is used\n"
          "                to encompass the polygon.\n"
          "   --mercator: Generate quadtree nodes assuming a Mercator\n"
          "               projection.\n",
          progn.c_str());
  exit(1);
}

int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    bool no_write = false;
    bool is_mercator = false;

    std::string kml_polygon_file;
    std::string qt_nodes_file;

    int max_level = 22;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("no_write", no_write);
    options.flagOpt("?", help);
    options.flagOpt("mercator", is_mercator);

    options.opt("max_level", max_level);
    options.opt("qt_nodes_file", qt_nodes_file);
    options.opt("kml_polygon_file", kml_polygon_file);
    options.opt("qt_nodes_file", qt_nodes_file);

    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
      return 1;
    }

    if (kml_polygon_file.empty() || !khIsFileReadable(kml_polygon_file)) {
      notify(NFY_WARN, "Cannot read polygon file: %s",
             kml_polygon_file.c_str());
      usage(progname);
      return 1;
    }

    if (qt_nodes_file.empty()) {
      notify(NFY_WARN, "Quadtree nodes output file not specified.");
      usage(progname);
      return 1;
    }

    // Try polygons in grids with different origins.
    fusion_portableglobe::Grid* grid;

    std::ofstream fout;
    fout.open(qt_nodes_file.c_str());
    fusion_portableglobe::Polygons polygons0(kml_polygon_file);
    fusion_portableglobe::Polygons polygons1(kml_polygon_file);
    for (size_t i = 0; i < polygons0.NumberOfPolygons(); ++i) {
      fusion_portableglobe::Grid grid0(-180.0, -90.0);
      fusion_portableglobe::Grid grid1(0.0, -90.0);

      grid0.AddPolygon(polygons0.GetPolygon(i));
      grid1.AddPolygon(polygons1.GetPolygon(i));

      // Select grid that has smallest bounding box,
      // since a polygon on a sphere defines two areas
      // and we have to assume one of them.
      std::string which_grid = "grid0";
      grid = &grid0;
      double area = grid0.BoundingBoxArea();
      if (grid1.BoundingBoxArea() + 0.01 < area) {
        std::string which_grid = "grid1";
        area = grid1.BoundingBoxArea();
        grid = &grid1;
      }

      notify(NFY_VERBOSE, "Using: %s (%f)", which_grid.c_str(), area);

      grid->CalculateQuadtreeNodesForPolygon(max_level, is_mercator, fout);
    }
    fout.close();
  } catch(const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
}

