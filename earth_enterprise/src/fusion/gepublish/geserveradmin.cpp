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


#include <stdlib.h>

#include "common/config/geConfigUtil.h"
#include "common/geConsoleProgress.h"
#include "common/khGetopt.h"
#include "common/geConsoleAuth.h"
#include "common/khSpawn.h"
#include "common/khException.h"
#include "autoingest/.idl/Systemrc.h"
#include "common/khStringUtils.h"
#include "common/khFileUtils.h"
#include "common/khConstants.h"
#include "common/khstl.h"
#include "fusion/autoingest/AssetVersion.h"
#include "fusion/dbmanifest/dbmanifest.h"
#include "fusion/gepublish/PublisherClient.h"

// TODO:
// - remove disablevs-functionality(?);
// - move enable/disable cutter to server middleware.
// - managing search definitions for publishing:
//   add search definition parameter in publish.

namespace {

// Location of example config files.
const std::string kGehttpdExampleDir = "/opt/google/share/gehttpd/examples/";

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  const std::string curr_host = khHostname();

  fprintf(
    stderr,
    "\n"
    "usage: %s [options] <command> \n"
    " --help                     Display this message.\n"
    "\n"
    "Options:\n"
    " --fusion_host              Fusion hostname. Default: %s\n"
    " --stream_server_url <url>  Default: http://%s\n"
    " --search_server_url <url>  DEPRECATED. Always set to stream_server_url.\n"
    " --server_type              Default: stream\n"
    "   <stream/search>          Required for the following commands:\n"
    "                            listdbs, dbdetails, garbagecollect.\n"
    " --cacert <file>            A certificate authority (CA) file path.\n"
    " --insecure                 Perform 'insecure' SSL connections and"
    "                            transfers.\n"
    "\n"
    "Database (DB) commands:\n"
    " --listdbs                  List all DBs registered on the server.\n"
    "   [--fusion_host <host>]   If --fusion_host is specified, only DBs\n"
    "                            from the <host> will be listed.\n"
    "   [--portable]             If --portable is specified, only portable\n"
    "                            DBs will be listed.\n"
    " --publisheddbs             List the DBs curently published on\n"
    "   [--portable]             the server.\n"
    " --listtgs                  List all target paths serving DBs.\n"
    " --dbdetails <db_name>      Preview details for the specified DB.\n"
    " --adddb <db_name>          Register a new DB. Optionally assign an\n"
    "   [--dbalias] <alias>      alias to make it easy to remember.\n"
    " --deletedb <db_name>       Delete a registered DB from the server.\n"
    " --pushdb <db_name> ...     Push one or more DBs with the given\n"
    "                            server. (Ex. --pushdb db1 --pushdb db2)\n"
    "   [--force_copy]           Copy DB files while pushing, otherwise\n"
    "                            create a hard/symbolic link whenever\n"
    "                            possible.\n"
    " --publishdb <db_name>      Publish a registered DB on the specified\n"
    "   --targetpath             target path.\n"
    "              <target_path> \n"
    "   [--vhname <vh_name>]     If virtual host is omitted,\n"
    "                            <db_name> is published to the default\n"
    "                            virtual host: \"%s\".\n"
    "   [--setecdefault]         Publish this database as the default one for\n"
    "                            the Earth Client to connect to if no database\n"
    "                            or virtual host is specified upon initial\n"
    "                            connection.\n"
    "   [--enable_poisearch]     Enable Point of Interest search if database\n"
    "                            contains POI data.\n"
    "   [--enable_enhancedsearch]If POI search is enabled, enable enhanced\n"
    "                            search.\n"
    "   [--serve_wms]            Serve content via WMS.\n"
    " --unpublish <target_path>  Unpublish the DB served from the specified\n"
    "                            target path.\n"
    " --republishdb <db_name>    Publish a registered DB on the specified\n"
    "   --targetpath             target un-publishing existing one and \n"
    "              <target_path> retaining same publish context \n"
    "                            (virtual host, search services, snippets).\n"
    "                            Target path needs to be an existing\n"
    "                            target path.\n"
    " --swaptargets              Swaps databases published on specified\n"
    "  --targetpath_a            targets.\n"
    "              <target_path>\n"
    "  --targetpath_b\n"
    "              <target_path>\n"
    " --targetdetails            Get publish context for a published target\n"
    "              <target_path> path, like snippets, search defs etc..\n"
    "\n"
    "Virtual Host (VH) commands:\n"
    " --listvhs                  List all registered VHs.\n"
    " --vhdetails <vh_name>      Preview details for a specific VH.\n"
    " --addvh <vh_name>          Register a new VH. \n"
    "   [--ssl]                  Create SSL VH (*.location_ssl config).\n"
    "   [--vhcachelevel <level>] Specifies a cache level (1 - 3, default: 2) for the VH.\n"
    "   [--vhurl <vh_url>]       The <vh_url> can be specified either as a complete\n"
    "                            URL, in the form of scheme://host:port/path\n"
    "                            (e.g. http://myserver.net:8080/my_security)\n"
    "                            or an absolute path (e.g. /my_security).\n"
    "                            If --vhurl is omitted, it will be set to\n"
    "                            '/<vh_name>_host'.\n"
    "                            In case of an absolute path, for building a complete\n"
    "                            URL when publishing, a server hostname will be derived\n"
    "                            from the Apache config. Also, for a regular VH,\n"
    "                            a port number will be derived from the Apache config,\n"
    "                            and for a SSL VH, a default port (443) will be used.\n"
    "                            Note: if you use non-default port for SSL VH or\n"
    "                            have multiple Listen ports in the Apache config,\n"
    "                            you have to specify --vhurl as an complete URL.\n"
    "                            Note: the <vh_url> must match the path in\n"
    "                            the corresponding VH config file (*.location[_ssl]).\n"
    " --deletevh <vh_name>       Delete a registered VH.\n"
    //    " --disablevs <vs_name>      Disable a registered VS.\n"
    //    "                            To re-enable the VS simply publish\n"
    //    "                            a DB to it.\n"
    "\n"
    "Admin commands:\n"
    " --garbagecollect           Free up unused space on the server.\n"
    " --portable_cleanup         Clean up portable globes registration\n"
    "                            information. Unregister/Unpublish portable\n"
    "                            globes that are missing in file system.\n"
    " --disable_cutter           [INTERNAL] Disable portable globe cutter in\n"
    "                            localhost.\n"
    "                            Use /opt/google/bin/gecutter disable.\n"
    " --enable_cutter            [INTERNAL] Enable portable globe cutter in\n"
    "                            localhost.\n"
    "                            Use /opt/google/bin/gecutter enable.\n"
    "\n"
    "Examples:\n"
    "Example of adding a virtual host:\n"
    " geserveradmin --addvh public_vh --vhurl http://mysite.com/public_vh\n"
    "Example of registering/pushing/publishing the latest version of a database:\n"
    "    geserveradmin --adddb databases/test\n"
    "    geserveradmin --pushdb databases/test\n"
    "    geserveradmin --publishdb databases/test --targetpath /test\n"
    "Example of registering/pushing/publishing a specific version of a database :\n"
    "    geserveradmin --adddb databases/test.kdatabase?version=36\n"
    "    geserveradmin --pushdb databases/test.kdatabase?version=36\n"
    "    geserveradmin --publishdb databases/test.kdatabase?version=36"
    " --targetpath /test\n"
    "    geserveradmin --republishdb databases/test.kdatabase?version=36"
    " --targetpath /test\n"
    "    geserveradmin --swaptargets --targetpath_a <target_path>"
    " --targetpath_b <target_path>\n"
    "    geserveradmin --targetdetails /test\n"
    "\n"
    "Note: you can use the short database name or the full path to specify\n"
    " the version of a gedb/mapdb directory when registering/pushing/publishing.\n"
    " The following publish commands are equivalent if version 36 of"
    " the test.kdatabase is the latest version:\n"
    " [1] geserveradmin --publishdb databases/test --vhname public"
    " --targetpath /test\n"
    " [2] geserveradmin --publishdb databases/test.kdatabase --vhname public"
    " --targetpath /test\n"
    " [3] geserveradmin --publishdb databases/test.kdatabase?version=36"
    " --vhname public --targetpath /test\n"
    " [4] geserveradmin --publishdb "
    "/gevol/assets/databases/test.kdatabase/gedb.kda/verXXX/gedb "
    " --vhname public --targetpath /test\n"
    " When specifying an absolute path [4], you must specify a path to\n"
    " a corresponding version of a gedb/mapdb directory. Note that the version\n"
    " number (verXXX) for gedb/mapdb asset may be different.\n"
    "Example of unpublishing a target path /test:\n"
    " geserveradmin --unpublish /test\n"
    "\n",
    progn.c_str(), curr_host.c_str(), curr_host.c_str(),
    kDefaultVirtualHost.c_str());
  exit(1);
}


// For the adddb/pushdb/publishdb command argument:
// 1) Converts the short publishdb path to the gedb_path if required.
// 2) Checks whether absolute gedb/mapdb path is correct.
// published_db: input path or short name of the db to publish.
// gedb_path: the full path to the versioned gedb or mapdb directory.
// for_delete: whether to use simple version check (fewer checks). We want to
// be able to delete old database versions even if we can't push/publish them.
void IdentifyPublishedDb(const std::string& publish_db,
                         std::string* gedb_path,
                         bool for_delete) {
  // By default, assume the input publishdb is the full gedb path.
  *gedb_path = publish_db;

  // 2 cases: publishddb is absolute path or short name.
  if (!khIsAbspath(*gedb_path)) {  // Short path name case.
    if (!khExists(Systemrc::Filename())) {
      throw khException("The short database name form of the --publishdb "
          "argument cannot be used with --publishdb on Server-only "
          "systems.\n"
          "Please retry using the long form of the name as shown in the "
          "usage example.");
    }
    if (for_delete) {
      // we call the simple (fewer checks) version because we want to be
      // able to delete old database versions even if we can't publish them.
      if (!AssetVersionImpl::SimpleGetGedbPath(publish_db, gedb_path)) {
        notify(NFY_FATAL, "Unable to get database path and type.");
      }
    } else {
      if (!AssetVersionImpl::GetGedbPath(publish_db, gedb_path)) {
        notify(NFY_FATAL, "Unable to get database path and type.");
      }
    }
  }
  *gedb_path = khNormalizeDir(*gedb_path, false);  // Remove trailing "/".

  std::string db_type;
  if (!khIsAbsoluteDbName(*gedb_path, &db_type)) {
    notify(NFY_FATAL,
           "Invalid or unable to get absolute gedb/mapdb path.");
  }
}

void SetFusionHost(PublisherClient* client,
                   const std::string& gedb_path,
                   const std::string& default_host) {
  std::string path = gedb_path;
  const std::string fusion_host = DbManifest(&path).FusionHost();
  client->SetFusionHost(fusion_host.empty() ? default_host : fusion_host);
}

void DisableCutter() {
  int ret_status =
    system("cd /opt/google/bin; "
           "chmod a-x gepolygontoqtnodes; "
           "chmod a-x gekmlgrabber; "
           "chmod a-x geportableglobebuilder; "
           "chmod a-x geportableglobepacker; "
           "cd /opt/google/gehttpd/cgi-bin; "
           "chmod a-r globe_cutter_app.py; "
           "cd /opt/google/gehttpd/htdocs; "
           "chmod a-r cutter/*.html; "
           "chmod -R a-r cutter/js; ");
  exit(ret_status);
}

void EnableCutter() {
  // Portable and Fusion are both served by fdb module, which should always
  // be available.
  int ret_status =
    system("cd /opt/google/bin; "
           "chmod a+x gepolygontoqtnodes; "
           "chmod a+x gekmlgrabber; "
           "chmod a+x geportableglobebuilder; "
           "chmod a+x geportableglobepacker; "
           "cd /opt/google/gehttpd/cgi-bin; "
           "chmod a+r globe_cutter_app.py; "
           "cd /opt/google/gehttpd/htdocs; "
           "chmod a+r cutter/*.html; "
           "chmod -R a+r cutter/js; ");
  exit(ret_status);
}

template <typename T, size_t N> inline size_t SizeOfArray(const T(&)[N]) {
  return N;
}

}  // end namespace


int main(int argc, char* argv[]) {
  try {
    std::string progname = argv[0];

    khGetopt options;
    bool help = false;
    bool enable_cutter = false;
    bool disable_cutter = false;
    bool listdbs = false;
    bool publisheddbs = false;
    bool ec_default_db = false;
    bool listtgs = false;
    bool only_portables = false;
    bool listvhs = false;
    bool garbagecollect = false;
    bool portable_cleanup = false;
    bool listsearchdefs = false;
    bool force_copy = false;
    bool vhssl = false;
    bool swaptargets = false;
    bool enable_poisearch = false;
    bool enable_enhancedsearch = false;
    bool serve_wms = false;
    std::string adddb, dbdetails, deletedb, publishdb,  unpublish;
    std::string republishdb, targetdetails;
    std::string target_path, dbalias;
    std::string target_path_a, target_path_b;
    std::string addvh, vhdetails, deletevh /*, disablevs,*/;
    std::string vhname, vhurl, vhcachelevel;
    std::vector<std::string> pushdbs;
    std::string fusion_host;
    const std::string curr_host = khHostname();
    const std::string default_url("http://" + curr_host);
    std::string stream_server_url(default_url);
    std::string search_server_url(default_url);
    std::string server_type = "stream";
    bool insecure = false;
    std::string cacert = "";

    options.flagOpt("help", help);
    options.flagOpt("enable_cutter", enable_cutter);
    options.flagOpt("disable_cutter", disable_cutter);
    options.opt("stream_server_url", stream_server_url);
    options.opt("search_server_url", search_server_url);
    options.flagOpt("insecure", insecure);
    options.opt("cacert", cacert);
    options.flagOpt("listdbs", listdbs);
    options.flagOpt("publisheddbs", publisheddbs);
    options.flagOpt("listtgs", listtgs);
    options.flagOpt("portable", only_portables);
    options.flagOpt("garbagecollect", garbagecollect);
    options.flagOpt("portable_cleanup", portable_cleanup);
    options.opt("dbdetails", dbdetails);
    options.opt("adddb", adddb);
    options.opt("dbalias", dbalias);
    options.opt("deletedb", deletedb);
    options.flagOpt("force_copy", force_copy);
    options.vecOpt("pushdb", pushdbs);
    options.opt("publishdb", publishdb);
    options.flagOpt("setecdefault", ec_default_db);
    options.flagOpt("enable_poisearch", enable_poisearch);
    options.flagOpt("enable_enhancedsearch", enable_enhancedsearch);
    options.flagOpt("serve_wms", serve_wms);
    options.opt("unpublish", unpublish);
    options.opt("republishdb", republishdb);
    options.flagOpt("swaptargets", swaptargets);
    options.opt("targetpath_a", target_path_a);
    options.opt("targetpath_b", target_path_b);
    options.opt("targetdetails", targetdetails);
    options.opt("targetpath", target_path);
    options.opt("vhname", vhname);
    options.flagOpt("listvhs", listvhs);
    options.opt("vhdetails", vhdetails);
    options.opt("addvh", addvh);
    options.opt("vhurl", vhurl);
    options.opt("vhcachelevel", vhcachelevel);
    options.flagOpt("ssl", vhssl);
    options.opt("deletevh", deletevh);
    //    options.opt("disablevs", disablevs);
    options.opt("listsearchdefs", listsearchdefs);
    options.opt("server_type", server_type);
    options.opt("fusion_host", fusion_host);

    // Note: All commands are exclusive.
    // Note: adding new command, the exclusive_commmand_set needs to be updated.
    std::string exclusive_command_set[] = {
      "help",
      "disable_cutter", "enable_cutter",
      "listdbs", "publisheddbs", "listtgs", "garbagecollect",
      "portable_cleanup",
      "adddb", "deletedb", "dbdetails", "pushdb", "publishdb", "unpublish",
      "republishdb", "swaptargets", "targetdetails",
      "addvh", "deletevh", "listvhs", "vhdetails",  /* "disablevs", */
      "listsearchdefs"};

    options.setExclusive(std::set<std::string>(
        &exclusive_command_set[0],
        &exclusive_command_set[SizeOfArray(exclusive_command_set)]));

    int argn;
    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    if ((enable_poisearch || enable_enhancedsearch) && !publishdb.size()) {
      throw khException("POI search and enhanced search are only used when publishing a database.\n");
    }

    if (enable_enhancedsearch && !enable_poisearch) {
      throw khException("Enhanced search cannot be enabled without POI search.\n");
    }

    if (serve_wms && publishdb.empty()) {
      throw khException("serve_wms can only be used with --publishdb.\n");
    }

    if (disable_cutter) {
      DisableCutter();
      return 0;
    }
    if (enable_cutter) {
      EnableCutter();
      return 0;
    }

    // Check if stream_server_url is correctly formatted.
    std::string stream_server_protocol;
    std::string stream_server_host;
    std::string stream_server_path;
    if (!UrlSplitter(stream_server_url,
                     &stream_server_protocol,
                     &stream_server_host,
                     &stream_server_path)) {
      std::string message = "The stream server URL " + stream_server_url +
          " is not correctly formatted.";
      throw khException(message);
    }

    // NOTE: The search_server_url is deprecated in GEE 5.0.0.
    // It is set equal to stream_server_url.
    if (search_server_url != stream_server_url) {
      if (search_server_url != default_url) {
        // A user set the search_server_url, and it is not equal to
        // the stream_server_url.
        notify(NFY_WARN,
               "The search_server_url is deprecated."
               " Setting it equal to stream_server_url:\n    %s.\n",
               stream_server_url.c_str());
      }
      search_server_url = stream_server_url;
    }

    // For publishing, we need to set the virtual servers to default names
    // if omitted.
    std::string default_virtual_host(kDefaultVirtualHost);
    if (vhname.empty())
      vhname = default_virtual_host;

    ServerConfig stream_server(stream_server_url, vhname, cacert,
                               insecure);
    geConsoleProgress progress(0, "bytes");
    geConsoleAuth auth("Authentication required. Press CTRL-C to abort.");

    PublisherClient publisher_client(
        fusion_host.empty() ? curr_host : fusion_host,
        stream_server, stream_server, &progress, &auth, force_copy);

    PublisherClient::ServerType stype =
      (server_type == "stream") ? PublisherClient::STREAM_SERVER :
      (server_type == "search") ? PublisherClient::SEARCH_SERVER :
                                  PublisherClient::UNKNOWN_SERVER;
    if (stype == PublisherClient::UNKNOWN_SERVER) {
      usage(progname, "Invalid server type.");
    }

    std::string err_msg;
    if (listdbs) {
      FusionDbInfoVector db_infos;
      PortableInfoVector portable_infos;
      const std::string& server = (stype == PublisherClient::STREAM_SERVER)
                                ? stream_server_url + " (stream)"
                                : search_server_url + " (search)";
      if (publisher_client.ListDatabases(stype, &db_infos, &portable_infos)) {
        if (fusion_host.empty()) {
          if (!only_portables) {
            fprintf(stdout, "\nThe %Zu database(s) registered on %s\n",
                    db_infos.size(), server.c_str());
            for (unsigned int i = 0; i < db_infos.size(); ++i) {
              const FusionDbInfo& db_info = db_infos[i];
              fprintf(
                  stdout,
                  "\n\tDatabase: %s\n\tFusion Host: %s\n\tDescription: %s\n",
                  db_info.path.c_str(),
                  db_info.fusion_hostname.c_str(),
                  db_info.description.c_str());
            }
          }

          fprintf(stdout, "\nThe %Zu portable(s) registered on %s\n",
                  portable_infos.size(), server.c_str());
          for (unsigned int i = 0; i < portable_infos.size(); ++i) {
            const PortableInfo& portable_info = portable_infos[i];
            fprintf(stdout, "\n\tPortable: %s\n\tDescription: %s\n",
                    portable_info.path.c_str(),
                    portable_info.description.c_str());
          }
        } else {
          fprintf(stdout,
                  "\nThe database(s) from Fusion host %s registered on %s\n",
                  fusion_host.c_str(), server.c_str());
          for (unsigned int i = 0; i < db_infos.size(); ++i) {
            const FusionDbInfo& db_info = db_infos[i];
            if (fusion_host == db_info.fusion_hostname) {
              fprintf(stdout, "\n\tDatabase: %s\n\tDescription: %s\n",
                      db_info.path.c_str(),
                      db_info.description.c_str());
            }
          }
        }
      } else {
        throw khException(publisher_client.ErrMsg());
      }
    } else if (publisheddbs) {
      PublishedFusionDbInfoVector published_db_infos;
      PublishedPortableInfoVector published_portable_infos;

      const std::string server = stream_server_url + " (stream)";
      if (publisher_client.PublishedDatabases(
              stype, &published_db_infos, &published_portable_infos)) {
        if (!only_portables) {
          fprintf(stdout, "\nThe %Zu database(s) published on %s\n",
                  published_db_infos.size(), server.c_str());
          for (unsigned int i = 0; i < published_db_infos.size(); ++i) {
            const PublishedFusionDbInfo& published_db_info =
                published_db_infos[i];
            fprintf(stdout,
                    "\nTarget Path: %s\n\tVirtual Host: %s\n\tDatabase: %s\n\t"
                    "Fusion Host: %s\n\tDescription: %s\n",
                    published_db_info.target_path.c_str(),
                    published_db_info.vh_name.c_str(),
                    published_db_info.path.c_str(),
                    published_db_info.fusion_hostname.c_str(),
                    published_db_info.description.c_str());
          }
        }

        fprintf(stdout, "\nThe %Zu portable(s) published on %s\n",
                published_portable_infos.size(), server.c_str());
        for (unsigned int i = 0; i < published_portable_infos.size(); ++i) {
          const PublishedPortableInfo& published_portable_info =
              published_portable_infos[i];
          fprintf(stdout,
                  "\nTarget Path: %s\n\tVirtual Host: %s\n\tPortable: %s"
                  "\n\tDescription: %s\n",
                  published_portable_info.target_path.c_str(),
                  published_portable_info.vh_name.c_str(),
                  published_portable_info.path.c_str(),
                  published_portable_info.description.c_str());
        }
      } else {
        throw khException(publisher_client.ErrMsg());
      }
    } else if (listtgs) {
      std::vector<std::string> target_paths;
      if (publisher_client.ListTargetPaths(stype, &target_paths)) {
        fprintf(stdout, "%Zu target path(s) serving published DB(s):\n",
                target_paths.size());
        for (size_t i = 0; i < target_paths.size(); ++i) {
          fprintf(stdout, "%Zu. %s\n", (i + 1), target_paths[i].c_str());
        }
      } else {
        throw khException(publisher_client.ErrMsg());
      }
    } else if (dbdetails.size()) {
      std::vector<std::string> file_names;
      if (publisher_client.QueryDatabaseDetails(
              stype, dbdetails, &file_names)) {
        fprintf(stdout, "Database: %s\n", dbdetails.c_str());
        for (unsigned int i = 0; i < file_names.size(); ++i) {
          fprintf(stdout, "%d.  %s\n", i+1, file_names[i].c_str());
        }
      } else {
        throw khException(publisher_client.ErrMsg());
      }
    } else if (targetdetails.size()) {
      std::vector<std::string> publish_context;
      if (publisher_client.QueryPublishContext(
              stype, targetdetails, &publish_context)) {
        fprintf(stdout, "Publish context for target path: %s\n", targetdetails.c_str());
        for (unsigned int i = 0; i < publish_context.size(); ++i) {
          fprintf(stdout, "\t%s\n", publish_context[i].c_str());
        }
        fprintf(stdout, "\n");
      } else {
        fprintf(stdout, "%s\n", publisher_client.ErrMsg().c_str());
        throw khException(publisher_client.ErrMsg());
      }
    } else if (adddb.size()) {
      fprintf(stdout, "Registering database: %s ...\n", adddb.c_str());
      fflush(stdout);
      std::string gedb_path;
      // Convert the db name to a full gedb path
      IdentifyPublishedDb(adddb, &gedb_path, false /* for_delete */);

      if (dbalias.empty())
        dbalias = adddb;
      if (fusion_host.empty()) {
        SetFusionHost(&publisher_client, gedb_path, curr_host);
      }
      if (publisher_client.AddDatabase(gedb_path, dbalias)) {
        fprintf(stdout, "Database registration successful.\n");
      } else {
        throw khException(publisher_client.ErrMsg() +
                          "\nDatabase registration failed.\n");
      }
    } else if (deletedb.size()) {
      fprintf(stdout, "Deleting database: %s ...\n", deletedb.c_str());
      fflush(stdout);
      // Convert the db name to a full gedb path
      std::string gedb_path;
      IdentifyPublishedDb(deletedb, &gedb_path, true /* for_delete */);

      if (publisher_client.DeleteDatabase(gedb_path)) {
        fprintf(stdout, "Database successfully deleted.\n");
      } else {
        throw khException(publisher_client.ErrMsg() +
                          "\nDatabase delete request failed.\n");
      }
    } else if (pushdbs.size()) {
      fprintf(stdout, "Pushing databases ...\n");
      fflush(stdout);
      for (size_t i = 0; i < pushdbs.size(); ++i) {
        const std::string& pushdb = pushdbs[i];
        std::string gedb_path;
        // Convert the db name to a full gedb path
        IdentifyPublishedDb(pushdb, &gedb_path, false /* for_delete */);
        if (fusion_host.empty()) {
          SetFusionHost(&publisher_client, gedb_path, curr_host);
        }
        if (publisher_client.PushDatabase(gedb_path)) {
          fprintf(stdout, "Database successfully pushed.\n");
        } else {
          throw khException(publisher_client.ErrMsg() +
                            "\nDatabase push request failed.\n");
        }
      }
    } else if (publishdb.size()) {
      fprintf(stdout, "Publishing database: %s ...\n", publishdb.c_str());
      fflush(stdout);
      if (target_path.empty()) {
        throw khException("Target path is not specified.\n");
      }

      // Convert the db name to a full gedb path.
      std::string gedb_path;
      if (!publishdb.empty()) {
        IdentifyPublishedDb(publishdb, &gedb_path, false /* for_delete */);
      }

      if (fusion_host.empty()) {
        SetFusionHost(&publisher_client, gedb_path, curr_host);
      }
      if (publisher_client.PublishDatabase(gedb_path, target_path, vhname, ec_default_db, enable_poisearch, enable_enhancedsearch, serve_wms)) {
        fprintf(stdout, "Database successfully published.  EC Default Database: %s\n", ec_default_db ? "true" : "false");
      } else {
        throw khException(publisher_client.ErrMsg() +
                          "\nDatabase publish request failed.\n");
      }
    } else if (republishdb.size()) {
      fprintf(stdout, "Republishing database: %s ...\n", republishdb.c_str());
      fflush(stdout);
      if (target_path.empty()) {
        throw khException("Target path is not specified.\n");
      }

      // Convert the db name to a full gedb path.
      std::string gedb_path;
      if (!republishdb.empty()) {
        IdentifyPublishedDb(republishdb, &gedb_path, false /* for_delete */);
      }

      if (publisher_client.RepublishDatabase(gedb_path, target_path)) {
        fprintf(stdout, "Database successfully republished.\n");
      } else {
        throw khException(publisher_client.ErrMsg() +
                          "\nDatabase republish request failed.\n");
      }
    } else if (swaptargets) {
      fprintf(stdout, "Swapping targets %s and %s ...\n",
              target_path_a.c_str(), target_path_b.c_str());
      fflush(stdout);

      if (target_path_a.empty() || target_path_b.empty()) {
        throw khException("target path is not specified.\n");
      }

      if (publisher_client.SwapTargets(target_path_a, target_path_b)) {
        fprintf(stdout, "Targets successfully swapped.\n");
      } else {
        throw khException(publisher_client.ErrMsg() +
                          "\nSwapTargets request failed.\n");
      }
    } else if (unpublish.size()) {
      fprintf(stdout, "Unpublishing target path: %s ...\n", unpublish.c_str());
      fflush(stdout);
      if (publisher_client.UnpublishDatabase(unpublish)) {
        fprintf(stdout, "Target path successfully unpublished.\n");
      } else {
        throw khException(publisher_client.ErrMsg() +
                          "\nUnpublish request failed.\n");
      }
    } else if (listvhs) {
      std::vector<std::string> vh_names, vh_urls;
      if (publisher_client.ListVirtualHosts(&vh_names, &vh_urls)) {
        fprintf(stdout, "Virtual Hosts ...\n");
        for (unsigned int i = 0; i < vh_names.size(); ++i) {
          fprintf(stdout, "%d. %s, %s\n", i+1, vh_names[i].c_str(),
                  vh_urls[i].c_str());
        }
      } else {
        throw khException(publisher_client.ErrMsg());
      }
    } else if (vhdetails.size()) {
      std::string vh_url;
      if (publisher_client.QueryVirtualHostDetails(vhdetails, &vh_url)) {
        if (vh_url.empty()) {
          fprintf(stdout, "Virtual Host: %s does not exist.\n",
                  vhdetails.c_str());
        } else {
          fprintf(stdout, "Virtual Host: %s\n", vhdetails.c_str());
          fprintf(stdout, "Url: %s\n", vh_url.c_str());
        }
      } else {
        throw khException(publisher_client.ErrMsg());
      }
    } else if (addvh.size()) {
      fprintf(stdout, "Registering Virtual Host: %s ...\n", addvh.c_str());
      fflush(stdout);

      bool is_port_based_vh = false;
      bool is_name_based_vh = false;

      if (vhurl.empty()) {
        vhurl = "/" + addvh + "_host";
      }
      assert(!vhurl.empty());
      // If VH url is complete URL, then validate it and check for compliance
      // with other options.
      if (vhurl[0] != '/') {
        // Validate that the VH url is properly formed.
        std::string vh_protocol;
        std::string vh_host;
        std::string vh_path;
        if (!UrlSplitter(vhurl, &vh_protocol, &vh_host, &vh_path)) {
          std::string message = "The virtual host url " + vhurl +
              " is not correctly formatted.";
          throw khException(message);
        }

        if (vhssl) {
          if (vh_protocol != "https") {
            std::string message = "The URL scheme " + vh_protocol +
                " is not compliant with SSL virtual host.";
            throw khException(message);
          }
        } else {
          if (vh_protocol != "http") {
            std::string message = "The URL scheme " + vh_protocol +
                " is not compliant with non SSL virtual host.";
            throw khException(message);
          }
        }

        std::vector<std::string> tokens;
        TokenizeString(vh_host, tokens, ":");
        if (tokens.size() == 2) {
          if (vhssl) {
            is_port_based_vh = tokens[1] != "443";
          } else {
            // True for anything other than port 80.
            is_port_based_vh = tokens[1] != "80";
          }
        }

        is_name_based_vh = vh_path.empty();
      }

      if (is_name_based_vh) {
        // TODO: add reference to configuration examples.
        fprintf(
            stdout,
            "\nWARN:\tName-based Virtual Host can't be created with this tool.\n\t"
            "You can create and register location-based virtual host with this tool,\n\t"
            "and then configure it.");
      } else {
        std::string conf_file =
            "<Apache path>/conf.d/virtual_servers/" + addvh;
        conf_file += vhssl ? "_host.location_ssl": "_host.location";

        if (is_port_based_vh) {
          fprintf(
              stdout,
           "\nNOTE:\tWhen creating location-based virtual host with "
           "non-default port number,\n\t you will need to verify inclusion of "
           "*.location[_ssl] in Apache config, and,\n\t"
           "if it is needed, modify file-extension and manually include\n\t"
           "created config file: %s\n\t"
           "into corresponding <VirtualHost>- section of Apache config, "
           "restart GEE server.\n\n",
            conf_file.c_str());
        }

        if (publisher_client.AddVirtualHost(
                addvh, vhurl, vhssl, vhcachelevel)) {
          fprintf(stdout, "Virtual Host registration successful.");
          fprintf(stdout,
                  "\nLocation-based Virtual Host created:\n\t %s\n",
                  conf_file.c_str());
        } else {
          throw khException(publisher_client.ErrMsg() +
                            "\nVirtual Host registration failed.\n");
        }
      }
    } else if (deletevh.size()) {
      fprintf(stdout, "Deleting Virtual Host: %s ...\n", deletevh.c_str());
      fflush(stdout);
      if (publisher_client.DeleteVirtualHost(deletevh)) {
        fprintf(stdout, "Virtual Host successfully deleted.\n");
      } else {
        throw khException(publisher_client.ErrMsg() +
                          "\nVirtual Host delete request failed.\n");
      }
#if 0
      // TODO: not sure if we need this, but it sounds like -
      // functionality of enabling/disabling serving of all the database served
      // by specific virtual host.
      // Whether it is required, seems, we can put handler settings into
      // runtime-config and clear content of runtime-config while disabling.
      // Server
    } else if (disablevs.size()) {
      fprintf(stdout, "Disabling virtual server: %s ...\n", disablevs.c_str());
      fflush(stdout);
      if (publisher_client.DisableVirtualServer(stype, disablevs)) {
        fprintf(stdout, "Virtual Server successfully disabled.\n");
      } else {
        throw khException(publisher_client.ErrMsg() +
                          "\nVirtual Server disabled request failed.\n");
      }
#endif
    } else if (listsearchdefs) {
      std::vector<std::string> searchdef_names, searchdef_contents;
      const std::string& server = search_server_url + " (search)";
      if (publisher_client.ListSearchDefs(&searchdef_names,
                                          &searchdef_contents)) {
        fprintf(stdout, "%Zu search definition(s) registered on %s\n",
                searchdef_names.size(), server.c_str());
        for (unsigned int i = 0; i < searchdef_names.size(); ++i) {
          fprintf(stdout, "\nSearch definition: %s\n\t Content:%s\n",
                  searchdef_names[i].c_str(), searchdef_contents[i].c_str());
        }
      } else {
        throw khException(publisher_client.ErrMsg());
      }
    } else if (garbagecollect) {
      std::uint32_t delete_count = 0;
      std::uint64_t delete_size = 0;
      std::string server = (stype == PublisherClient::STREAM_SERVER) ?
          stream_server_url + " (stream)":
          search_server_url + " (search)";
      if (publisher_client.GarbageCollect(stype, &delete_count, &delete_size)) {
        fprintf(stdout, "\nGarbage Collection completed on %s"
                "\n\t%d file(s) deleted\n\t%Zu byte(s) freed\n",
                server.c_str(), delete_count, delete_size);
      } else {
        throw khException(publisher_client.ErrMsg());
      }
    } else if (portable_cleanup) {
      std::string cleaned_portables_data;
      std::vector<std::string> cleaned_portables;
      std::string server = stream_server_url + " (stream)";
      if (publisher_client.Cleanup(PublisherClient::STREAM_SERVER,
                                   &cleaned_portables_data)) {
        // Note: format of returned string is
        // "Data: [{"path": "/opt/google/gehttpd/cutter/globes/mymap.glm",
        //          "name": "mymap.glm"},..]"
        // After tokenizer we will get list of tokens: "Data: [", '"path":',
        // "/opt/google/gehttpd/cutter/globes/mymap.glm", '"name":',
        // "mymap.glm", ..., "]".
        // Below, '-2' is to skip first and last items: "Data: [", and "]".
        if (!cleaned_portables_data.empty()) {
          TokenizeString(cleaned_portables_data,
                         cleaned_portables,
                         std::string("\": ,{}"));
          fprintf(stdout, "\nCleanup completed on %s.", server.c_str());
          if (cleaned_portables.size() > 1) {
            fprintf(stdout,
                    "\n%zu portable(s) unpublished/unregistered:",
                    (cleaned_portables.size() - 2)/4);
            for (size_t idx = 1; idx < cleaned_portables.size() - 2; idx += 4) {
              // Note: reporting just path value.
              fprintf(stdout, "\n\t %s", cleaned_portables[idx + 1].c_str());
            }
          } else {
            fprintf(stdout,
                    "\nNo portables unpublished/unregistered.");
          }
          fprintf(stdout, "\n");
        }
      } else {
        throw khException(publisher_client.ErrMsg());
      }
    } else {
      usage(progname, "Please specify a command.");
    }
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}
