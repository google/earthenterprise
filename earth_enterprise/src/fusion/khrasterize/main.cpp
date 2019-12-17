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

#include <string.h>
#include <cstdio>
#include "globals.h"
#include "rasterize.h"
#include <string>

using namespace std;

// Global declarations
struct CmdLineOptions {
  string  xyzFile;
  string  bilFile;
  string  hdrFile;
  string  geoFile;
  double  resolution;
  int     fillRadius;
  bool    verbose;
  int     method;

  CmdLineOptions () {
    xyzFile = bilFile = hdrFile = geoFile = string("");
    resolution = 0;
    fillRadius = 3;
    verbose = true;
    method = 0;
  }
};


VPoints ReadXYZ(string xyzFile)
{
  VPoints points;
  char buff[1000];
  FILE *fp = fopen(xyzFile.c_str(), "rt");
  if (fp == 0) {
    fprintf(stderr, "Unable to open file: %s\n", xyzFile.c_str());
    return points;
  }
  Point aPoint;
  int badCount=0;
  while (!feof(fp)) {
    buff[0] = 0;
    aPoint.x=aPoint.y=aPoint.z=-INFINITY;
    fgets(buff, 1000, fp);
    sscanf(buff, "%lf,%lf,%lf\n", &(aPoint.x), &(aPoint.y), &(aPoint.z));
    if (aPoint.x<=180 && aPoint.x>=-180 && aPoint.y<=90 && aPoint.y>=-90) {
      points.push_back(aPoint);
    }
    else {
      badCount++;
    }
  }
  if (badCount > 0) {
    fprintf(stderr, "Notice: %d Invalid Point(s)\n", badCount);
  }
  fclose(fp);

  return points;
}



int WriteBilFile(Grid&  grid,
                 string bilFile )
{
  FILE *fp = fopen(bilFile.c_str(), "wb");
  if (fp == 0) {
    fprintf(stderr, "Error: Unable to open output .bil file\n");
    return -1;
  }
  short* matrix = new short[grid.rows*grid.cols];
  int i, j, k=0;
  // the bil file is arranged N-S
  // so we need to flip ud our grid
  for (i=grid.rows-1; i>=0; i--) {
    for (j=0; j<grid.cols; j++) {
      matrix[k++] = short(grid.z[i][j]);
    }
  }
  int numWritten = fwrite(matrix, sizeof(short), grid.rows*grid.cols, fp);
  if (numWritten < 1) {
    fprintf(stderr, "Error: Cannot write output .bil file\n");
    delete[] matrix;
    fclose(fp);
    return -1;
  }

  delete[] matrix;
  fclose(fp);

  return 0;
}


int WriteHdrFile(Grid&  grid,
                 string hdrFile )
{
  FILE *fp = fopen(hdrFile.c_str(), "wb");
  if (fp == 0) {
    fprintf(stderr, "Error: Unable to open output .hdr file\n");
    return -1;
  }
  fprintf(fp, "BYTEORDER \t I\n");
  fprintf(fp, "LAYOUT \t BIL\n");
  fprintf(fp, "NROWS \t %d\n", grid.rows);
  fprintf(fp, "NCOLS \t %d\n", grid.cols);
  fprintf(fp, "NBANDS \t 1\n");
  fprintf(fp, "NBITS \t 16\n");
  fprintf(fp, "BANDROWBYTES \t %d\n", grid.cols*2);
  fprintf(fp, "TOTALROWBYTES \t %d\n", grid.cols*2);
  fprintf(fp, "TOTALROWBYTES \t %d\n", grid.cols*2);
  fprintf(fp, "TOTALROWBYTES \t %d\n", grid.cols*2);
  fprintf(fp, "BANDGAPBYTES  0\n");
  fprintf(fp, "NODATA \t -32768\n");
  fprintf(fp, "ULXMAP \t %3.10f\n", grid.xmin);
  fprintf(fp, "ULYMAP \t %3.10f\n", grid.ymin+(grid.rows-1)*grid.resolution);
  fprintf(fp, "XDIM \t %1.10f\n", grid.resolution);
  fprintf(fp, "YDIM \t %1.10f\n", grid.resolution);

  fclose(fp);
  return 0;
}



int WriteGeoFile(Grid&  grid,
                 string geoFile )
{
  FILE *fp = fopen(geoFile.c_str(), "wb");
  if (fp == 0) {
    fprintf(stderr, "Error: Unable to open output .geo file\n");
    return -1;
  }
  fprintf(fp, "north %3.10f\n", grid.ymin+(grid.rows-1)*grid.resolution);
  fprintf(fp, "south %3.10f\n", grid.ymin);
  fprintf(fp, "east %3.10f\n", grid.xmin+(grid.cols-1)*grid.resolution);
  fprintf(fp, "west %3.10f\n", grid.xmin);

  fclose(fp);
  return 0;
}


void PrintUsage()
{
  fprintf(stderr, "Usage: khrasterize [options] -r target_resolution -i xyz_file [-o bil_file] \n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t -q : quiet mode\n");
  fprintf(stderr, "\t -f : fill radius (default=3)\n");
  fprintf(stderr, "\t -m : interpolation method\n");
  fprintf(stderr, "\t\t 0: Nearest Neighbor (default)\n");
  fprintf(stderr, "\t\t 1: Linear\n");
  fprintf(stderr, "The input XYZ file contains comma separated 3D coordinates\n");
  fprintf(stderr, "NOTICE: This utility is currently under development.\n");

  return;
}


int ProcessCmdLineArgs(int argc,
                       char **argv,
                       CmdLineOptions& cmd)
{
  if (argc < 5) {
    PrintUsage();
    return -1;
  }

  int i;
  char buff[1024];

  for (i=1; i<argc && argv[i][0]=='-'; i++) {
    if (strcmp(argv[i], "-q") == 0) {
      cmd.verbose = false;
    }
    else if (strcmp(argv[i], "-r") == 0) {
      sscanf(argv[++i], "%lf", &(cmd.resolution));
    }
    else if (strcmp(argv[i], "-f") == 0) {
      sscanf(argv[++i], "%d", &(cmd.fillRadius));
    }
    else if (strcmp(argv[i], "-m") == 0) {
      sscanf(argv[++i], "%d", &(cmd.method));
    }
    else if (strcmp(argv[i], "-i") == 0) {
      sscanf(argv[++i], "%1023s", buff);
      cmd.xyzFile = string(buff);
    }
    else if (strcmp(argv[i], "-o") == 0) {
      sscanf(argv[++i], "%1023s", buff);
      cmd.bilFile = string(buff);
    }
    else {
      printf("Error: Invalid option '%s'\n", argv[i]);
      return -1;
    }
  }
  if (cmd.bilFile == "") {
    cmd.bilFile = cmd.xyzFile;
    string::size_type index = cmd.bilFile.find('.');
    if (index < cmd.bilFile.npos) {
      cmd.bilFile.replace(index, 4, ".bil");
    }
  }
  cmd.hdrFile = cmd.bilFile;
  string::size_type index = cmd.hdrFile.find('.');
  if (index < cmd.hdrFile.npos) {
    cmd.hdrFile.replace(index, 4, ".hdr");
  }

  cmd.geoFile = cmd.bilFile;
  index = cmd.geoFile.find('.');
  if (index < cmd.geoFile.npos) {
    cmd.geoFile.replace(index, 4, ".geo");
  }


  return 0;
}




int main(int argc, char* argv[])
{
  CmdLineOptions cmd;
  if (ProcessCmdLineArgs(argc, argv, cmd) < 0) {
    return -1;
  }

  VPoints points = ReadXYZ(cmd.xyzFile);
  int numPoints = points.size();
  if (cmd.verbose) {
    fprintf(stdout, "# points: %d\n", numPoints);
  }
  if (numPoints <= 0) {
    return -1;
  }

  Grid grid = Rasterize(points, cmd.resolution, cmd.fillRadius, cmd.method, cmd.verbose);
  if (WriteBilFile(grid, cmd.bilFile) < 0) {
    return -1;
  }
  if (WriteHdrFile(grid, cmd.hdrFile) < 0) {
    return -1;
  }
  if (WriteGeoFile(grid, cmd.geoFile) < 0) {
    return -1;
  }



  return 0;
}
