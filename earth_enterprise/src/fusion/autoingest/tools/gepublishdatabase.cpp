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


#include <stdio.h>
#include <stdlib.h>
#include <khGetopt.h>
#include <fusion/dbmanifest/dbmanifest.h>
#include <notify.h>
#include <geConsoleProgress.h>
#include <geConsoleAuth.h>
#include <common/khConstants.h>
#include <gepublish/PublisherClient.h>
#include <autoingest/.idl/ServerCombination.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <fusion/autoingest/AssetVersion.h>

std::string FusionHostName() {
  return AssetDefs::MasterHostName();
}

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

  std::string default_url = FusionHostName();
  fprintf(
    stderr,
    "usage: %s [options] <command> \n"
    " Options:\n"
    "  --help                  Display this usage message.\n"
    "\n"
    " Database (DB) commands:\n"
    "  --listdbs               List all DBs registered on the server.\n"
    "    [--serverurl] <url>   Use specific server URL.\n"
    "                          Default: http://%s\n"
    "  --publisheddbs          List all the published DBs on the server.\n"
    "    [--serverurl] <url>   Use specific server URL.\n"
    "                          Default: http://%s\n"
    "  --publish <db_name>     Publish a DB. Version number may be part of\n"
    "                          the <db_name> (test.kdatabase?version=6).\n"
    " e.g /gevol/assets/db/test.kdatabase/gedb.kda/ver074/gedb\n"
    " e.g db/test.kdatabase (for latest version)\n"
    " e.g 'db/test.kdatabase?version=74' (for 74th version, quoting for ?)\n"
    "    [--report_size_only]  Only report the published DB size.\n"
    "    [--server] <nickname> Use specific server association. If omitted,\n"
    "                          the default server association will be used\n"
    "                           \"%s\" for Earth databases and\n"
    "                           \"%s\" for Map databases.\n"
    "  --delete <db_name>      Delete a registered DB from the server.\n"
    "    [--serverurl] <url>   Use specific server URL.\n"
    "                          Default: http://%s\n"
    "Examples:\n"
    " Publish to the default server:\n"
    "  gepublishdatabase  --publish db/test.kdatabase\n"
    " Publish to a specific server:\n"
    "  gepublishdatabase  --publish db/test.kdatabase --server default_ge\n"
    " Publish a specific database version (version 6 here) to a specific server:\n"
    "  gepublishdatabase  --publish db/test.kdatabase?version=6 --server default_ge\n"
    " Delete a published database from the server:\n"
    "  Note: this assumes the virtual server is disabled via geserveradmin.\n"
    "  gepublishdatabase  --delete db/test.kdatabase\n"
    "\n",
    progn.c_str(), default_url.c_str(), default_url.c_str(),
    kDefaultEarthVirtualServer.c_str(), kDefaultMapVirtualServer.c_str(),
    default_url.c_str());
  exit(1);
}

// server_combination server combination whether it is found.
// return whether server combination is found.
bool GetServerCombination(ServerCombination *server_combination,
                          std::string *server,
                          const std::string &db_type) {
  assert(server_combination);

  std::string default_server(kDefaultEarthVirtualServer);
  ServerCombination::CombinationType ctype = ServerCombination::TYPE_GE;
  if (db_type == kMapDatabaseSubtype ||
      db_type == kMercatorMapDatabaseSubtype) {
    ctype = ServerCombination::TYPE_MAP;
    default_server = kDefaultMapVirtualServer;
  }
  if (server->empty())
    *server = default_server;

  bool server_found = false;

  ServerCombinationSet sc_set;
  sc_set.Load();
  if (sc_set.combinations.size() == 0) {
    notify(NFY_WARN,
           "Please add at least one server combination"
           " using the Publisher tool.");
    return server_found;
  }

  for (unsigned int i = 0; i < sc_set.combinations.size() && !server_found; ++i) {
    if (*server == sc_set.combinations[i].nickname) {
      if (sc_set.combinations[i].type == ctype) {
        *server_combination = sc_set.combinations[i];
        server_found = true;
      } else {
        notify(NFY_WARN,
               "Server association type does not match the database type.");
      }
    }
  }

  return server_found;
}

int
main(int argc, char *argv[]) {

  try {
    std::string progname = argv[0];
    // process commandline options
    int argn;
    bool help = false;
    bool listdbs = false;
    bool publisheddbs = false;
    bool report_size_only = false;
    std::string publishdbname, deletedbname, server;
    std::string serverurl = "http://" + FusionHostName();
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("listdbs", listdbs);
    options.flagOpt("publisheddbs", publisheddbs);
    options.opt("publish", publishdbname);
    options.flagOpt("report_size_only", report_size_only);
    options.opt("delete", deletedbname);
    options.opt("server", server);
    options.opt("serverurl", serverurl);

    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    ServerCombination server_combination;
    geConsoleProgress progress(0, "bytes");
    geConsoleAuth auth("Authentication required. Press CTRL-C to abort.");
    if (listdbs) {
      std::vector<std::string> db_names, db_pretty_names, host_names;
      server_combination.stream.url = serverurl;
      PublisherClient publisher_client(FusionHostName(),
                                       server_combination.stream,
                                       server_combination.search,
                                       &progress, &auth);
      if (publisher_client.ListDatabases(PublisherClient::STREAM_SERVER,
                                         db_names, db_pretty_names, host_names)) {
        fprintf(stdout, "%d database(s) registered on %s\n",
                        (int)db_pretty_names.size(), serverurl.c_str());
        for (unsigned int i = 0; i < db_pretty_names.size(); ++i) {
          fprintf(stdout, "\n\tDatabase: %s\n\tFusion Host:%s\n",
                          db_pretty_names[i].c_str(), host_names[i].c_str());
        }
      } else {
        fprintf(stderr, "%s\n", publisher_client.ErrMsg().c_str());
        return -1;
      }
    } else if (publisheddbs) {
      std::vector<std::string> db_names, db_pretty_names, host_names, vs_names;
      server_combination.stream.url = serverurl;
      PublisherClient publisher_client(FusionHostName(),
                                       server_combination.stream,
                                       server_combination.search,
                                       &progress, &auth);
      if (publisher_client.PublishedDatabases(PublisherClient::STREAM_SERVER,
            vs_names, db_names, db_pretty_names, host_names)) {
        fprintf(stdout, "%d database(s) published on %s\n",
                        (int)db_pretty_names.size(), serverurl.c_str());
        for (unsigned int i = 0; i < vs_names.size(); ++i) {
          fprintf(stdout, "\nVirtual Server: %s\n\tDatabase: %s\n\tFusion Host: %s\n",
                  vs_names[i].c_str(), db_pretty_names[i].c_str(), host_names[i].c_str());
        }
      } else {
        fprintf(stderr, "%s\n", publisher_client.ErrMsg().c_str());
        return -1;
      }
    } else if (publishdbname.size()) {
      std::string gedb_path, db_type, db_ref;
      if (!AssetVersionImpl::GetGedbPathAndType(
          publishdbname, &gedb_path, &db_type, &db_ref)) {
        notify(NFY_FATAL,
               "Publish Failed: Unable to get database path and type.");
      }
      if (report_size_only) {
        char buff[4096];
        snprintf(buff, sizeof(buff),
                 "gedisconnectedsend --sendpath %s --report_size_only",
                 gedb_path.c_str());
        exit(system(buff));
      }

      if (!GetServerCombination(&server_combination, &server, db_type))
        notify(NFY_FATAL,
               "Cannot find server association: %s.", server.c_str());

      std::string gedb_path_tmp = gedb_path;

      PublisherClient publisher_client(DbManifest(&gedb_path_tmp).FusionHost(),
                                       server_combination.stream,
                                       server_combination.search,
                                       &progress, &auth);

      // TODO: currently, we add, push and publish the DB here.
      // Are we going to have only publishing in this tool?
      if (!publisher_client.AddDatabase(gedb_path, db_ref)) {
        notify(NFY_FATAL, "Publish Failed: %s ", publisher_client.ErrMsg().c_str());
      }

      if (!publisher_client.PublishDatabase(gedb_path)) {
        notify(NFY_FATAL, "Publish Failed: %s ", publisher_client.ErrMsg().c_str());
      }

      fprintf(stdout, "Database Successfully Published.\n");
    } else if (deletedbname.size()) {
      std::string gedb_path;
      // we call the simple (fewer checks) version because we want to be
      // able to delete old database versions even if we can't publish them.
      if (!AssetVersionImpl::SimpleGetGedbPath(deletedbname, &gedb_path)) {
        notify(NFY_FATAL,
               "Delete Failed: Unable to interpret DB '%s'.",
               deletedbname.c_str());
      }

      server_combination.stream.url = serverurl;
      server_combination.search.url = serverurl;
      PublisherClient publisher_client(FusionHostName(),
                                       server_combination.stream,
                                       server_combination.search,
                                       &progress, &auth);
      if (publisher_client.DeleteDatabase(gedb_path)) {
        fprintf(stdout, "Database successfully deleted.\n");
      } else {
        notify(NFY_FATAL, "Delete Failed: %s ", publisher_client.ErrMsg().c_str());
      }
    } else {
      usage(progname, "Please specify a command.");
    }

  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
