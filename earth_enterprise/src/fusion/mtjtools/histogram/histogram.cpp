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

#include "stdio.h"

// GDAL includes
#include "gdal_priv.h"

// KHistogram class
#include "khistogram.h"

//
// Compute histogram of a GDAL image
//
int main(int argc, char* argv[])
{
  GDALSetCacheMax(512*1024*1024);
  GDALAllRegister();

  int format = 3; // most compressed. format 4 is Mark Aubin's format
  int merge = 0;
  int verbose = 0;
  const size_t buflen = 1024;
  char output[buflen] = "histogram.txt";
  char input[buflen] = {0};
  char *filelist = NULL;

  // parse command line 'option arguments'
  int cursor;
  for (cursor = 1; cursor < argc && argv[cursor][0] == '-'; cursor++)
  {
    if (strcmp(argv[cursor], "-filelist") == 0)
    {
      filelist = argv[++cursor];
    }
    else if (strcmp(argv[cursor], "--filelist") == 0)
    {
      filelist = argv[++cursor];
    }
    else if (strcmp(argv[cursor], "-f") == 0)
    {
      if (sscanf(argv[cursor], "-f%d", &format) != 1)
        sscanf(argv[++cursor], "%d", &format);
    }
    else if (strcmp(argv[cursor], "-i") == 0)
    {
      if (sscanf(argv[cursor], "-i%1023s", input) != 1)
        sscanf(argv[++cursor], "%1023s", input);
    }
    else if (strcmp(argv[cursor], "-o") == 0)
    {
      if (sscanf(argv[cursor], "-o%1023s", output) != 1)
        sscanf(argv[++cursor], "%1023s", output);
    }
    else if (strcmp(argv[cursor], "-m") == 0)
    {
      merge = 1;
    }
    else if (strcmp(argv[cursor], "-v") == 0)
    {
      verbose = 1;
    }
    else
    {
      printf("error: unknown option '%s'\n", argv[cursor]);
      return 1;
    }
  }

  // process remaining argument, if any
  if (cursor < argc && input[0] == '\0')
    strncpy(input, argv[cursor++], buflen);

  // validate configuration
  if ((filelist == NULL || filelist[0] == '\0') && input[0] == '\0')
  {
    printf("usage: histogram [-f format][-i input][-o output][-m][-filelist name] [input]\n");
    printf("  default format is 3 (most compressed representation)\n");
    printf("  default output file name is 'histogram.txt'\n");
    return 2;
  }

  if (filelist != 0)
  {
    // open file list
    FILE *fp;
    if ((fp = fopen(filelist, "r")) == NULL)
    {
      printf("error: unable to open filelist '%s'\n", filelist);
      exit(2);
    }

    // process each file in list
    if (merge == 0)
    {
      char nextName[2048];
      while (fscanf(fp, "%2047s", nextName) == 1)
      {
        if (verbose)
          printf("%s\n", nextName);

        // compute histogram
        KHistogram h;
        if (h.histogramGDAL(nextName))
        {
          printf("error: unable to compute histogram from file '%s'\n", nextName);
          return 1;
        }

        char *dot = strrchr(nextName, '.');
        if (dot != NULL)
        {
          *dot = '\0';
        }
        snprintf(output, buflen, "%s.his", nextName);

        // write histogram
        if (h.write(output, format))
        {
          printf("error: unable to write histogram to file '%s'\n", output);
          return 2;
        }
      }
    }
    else
    {
      char nextName[2048];
      KHistogram m;

      while (fscanf(fp, "%2047s", nextName) == 1)
      {
        KHistogram h;

        if (verbose)
          printf("%s\n", nextName);

        // histogram or file?
        char *dot = strrchr(nextName, '.');
        if (dot != NULL && strncmp(dot, ".his", 4) == 0)
        {
          if (h.read(nextName) == 0)
            m.accumulate(h);
          else
            printf("warning: unable to load histogram from file '%s'\n", nextName);
        }
        else
        {
          if (h.histogramGDAL(nextName) == 0)
            m.accumulate(h);
          else
            printf("warning: unable to compute histogram from file '%s'\n", nextName);
        }
      }

      // write histogram
      if (m.write(output, format))
      {
        printf("error: unable to write histogram to file '%s'\n", output);
        return 2;
      }
    }
    // close file list
    fclose(fp);
  }
  else
  {
    if (merge == 0)
    {
      // compute histogram
      KHistogram h;
      if (h.histogramGDAL(input))
      {
        printf("error: unable to compute histogram from file '%s'\n", input);
        return 1;
      }

      // write histogram
      if (h.write(output, format))
      {
        printf("error: unable to write histogram to file '%s'\n", output);
        return 2;
      }
    }
    else
    {
      KHistogram m;

      for (--cursor; cursor < argc; cursor++)
      {
        KHistogram h;

        if (verbose)
          printf("%s\n", argv[cursor]);

        // histogram or file?
        char *dot = strrchr(argv[cursor], '.');
        if (dot != NULL && strncmp(dot, ".his", 4) == 0)
        {
          if (h.read(argv[cursor]) == 0)
            m.accumulate(h);
          else
            printf("warning: unable to load histogram from file '%s'\n", argv[cursor]);
        }
        else
        {
          if (h.histogramGDAL(argv[cursor]) == 0)
            m.accumulate(h);
          else
            printf("warning: unable to compute histogram from file '%s'\n", argv[cursor]);
        }
      }

      // write histogram
      if (m.write(output, format))
      {
        printf("error: unable to write histogram to file '%s'\n", output);
        return 2;
      }
    }
  }

  // normal termination
  return 0;
}
