#!/usr/bin/perl -w-
#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


use strict;
use FindBin;
use Getopt::Long;
use File::Basename;
use lib $FindBin::Bin;
use AssetGen;

my $help = 0;
our $thiscommand = "@ARGV";

sub usage() {
    die "usage: $FindBin::Script <.srcfile> <outputfile>\n";
}
GetOptions("help|?"    => \$help) || usage();
usage() if $help;

my $srcfile = shift;
if (!$srcfile) {
    warn "No .srcfile specified\n";
    usage();
}

my $public_cpp = shift;
if (!$public_cpp) {
    warn "No output file specified\n";
    usage();
}


# ****************************************************************************
# ***  Read .src file
# ****************************************************************************
my %config;
ReadSrcFile($srcfile, \%config);


# *****************************************************************************
# ***  ${name}AssetImpl.h
# ****************************************************************************#
EnsureDirExists($public_cpp);
my $fh;
chmod 0777, $public_cpp;
open($fh, "> $public_cpp") || die "Unable to open $public_cpp: $!\n";
EmitAutoGeneratedWarning($fh);

print $fh <<EOF;


#include "${name}Asset.h"
#include <autoingest/AssetThrowPolicy.h>
#include <khFileUtils.h>
#include <khGuard.h>
#include <khxml/khdom.h>
using namespace khxml;


// ****************************************************************************
// ***  Supplied from ${name}.src
// ****************************************************************************
$config{"Asset.cpp"}


// ****************************************************************************
// ***  ${name}AssetImpl - Auto generated
// ****************************************************************************
namespace {
    void GetConfig(DOMElement *elem, $config &config);
}

extern void FromElement(DOMElement *elem, AssetStorage &self);

khRefGuard<${name}AssetImpl>
${name}AssetImpl::NewFromDOM(void *e)
{
    DOMElement *elem = (DOMElement*)e;
    AssetStorage storage;
    Config      config;
    FromElement(elem, storage);
    GetConfig(elem, config);
    return NewFromStorage(storage, config);
}


khRefGuard<${name}AssetImpl>
${name}AssetImpl::NewInvalid(const SharedString &ref)
{
    AssetStorage storage;
    Config      config;
    storage.SetInvalid(ref);
    return NewFromStorage(storage, config);
}



khRefGuard<${name}AssetImpl>
${name}AssetImpl::Load(const SharedString &ref)
{
    std::string filename = XMLFilename(ref);
    khRefGuard<${name}AssetImpl> result;
    time_t timestamp = 0;
    uint64 filesize = 0;

    if (khGetFileInfo(filename, filesize, timestamp) && (filesize > 0)) {
        std::unique_ptr<GEDocument> doc = ReadDocument(filename);
        if (doc) {
            try {
                DOMElement *top = doc->getDocumentElement();
                if (!top) {
                    throw khException(kh::tr("No document element"));
                }
                std::string tagname = FromXMLStr(top->getTagName());
                if (tagname != "${name}Asset") {
                    throw khException(kh::tr("Expected '%1', found '%2'")
                        .arg(ToQString("${name}Asset"),
                        ToQString(tagname)));
                }
                result = NewFromDOM(top);
            } catch (const std::exception &e) {
                AssetThrowPolicy::WarnOrThrow
                    (kh::tr("Error loading %1: %2")
                    .arg(ToQString(filename), QString::fromUtf8(e.what())));
            } catch (...) {
                AssetThrowPolicy::WarnOrThrow(kh::tr("Unable to load ")
                    + filename);
            }
        } else {
            AssetThrowPolicy::WarnOrThrow(kh::tr("Unable to read ")
                + filename);
        }
    } else {
        AssetThrowPolicy::WarnOrThrow(kh::tr("No such file: ") + filename);
    }


    if (!result) {
        result = NewInvalid(ref);
        // leave timestamp alone
        // if it failed but there was a vlid timestamp we want
        // to remember the timestamp of the one that failed
    }

    // store the timestamp so the cache can check it later
    result->timestamp = timestamp;
    result->filesize  = filesize;

    return result;
}

// ****************************************************************************
// ***  ${name}AssetVersionImpl - Auto generated
// ****************************************************************************
extern void FromElement(DOMElement *elem, AssetVersionStorage &self);

khRefGuard<${name}AssetVersionImpl>
${name}AssetVersionImpl::NewFromDOM(void *e)
{
    DOMElement *elem = (DOMElement*)e;
    AssetVersionStorage storage;
    Config      config;
    FromElement(elem, storage);
    GetConfig(elem, config);
    return NewFromStorage(storage, config);
}

khRefGuard<${name}AssetVersionImpl>
${name}AssetVersionImpl::NewInvalid(const SharedString &ref)
{
    AssetVersionStorage storage;
    Config      config;
    storage.SetInvalid(ref);
    return NewFromStorage(storage, config);
}

std::string ${name}AssetVersionImpl::PluginName(void) const {
  return "${name}";
}

khRefGuard<${name}AssetVersionImpl>
${name}AssetVersionImpl::Load(const SharedString &boundref)
{
    std::string filename = XMLFilename(boundref);
    khRefGuard<${name}AssetVersionImpl> result;
    time_t timestamp = 0;
    uint64 filesize = 0;

    if (khGetFileInfo(filename, filesize, timestamp) && (filesize > 0)) {
        std::unique_ptr<GEDocument> doc = ReadDocument(filename);
        if (doc) {
            try {
                DOMElement *top = doc->getDocumentElement();
                if (!top) {
                    throw khException(kh::tr("No document element"));
                }
                std::string tagname = FromXMLStr(top->getTagName());
                if (tagname != "${name}AssetVersion") {
                    throw khException(kh::tr("Expected '%1', found '%2'")
                        .arg(ToQString("${name}AssetVersion"),
                        ToQString(tagname)));
                }
                result = NewFromDOM(top);
            } catch (const std::exception &e) {
                AssetThrowPolicy::WarnOrThrow
                    (kh::tr("Error loading %1: %2")
                    .arg(ToQString(filename), QString::fromUtf8(e.what())));
            } catch (...) {
                AssetThrowPolicy::WarnOrThrow(kh::tr("Unable to load ")
                    + filename);
            }
        } else {
            AssetThrowPolicy::WarnOrThrow(kh::tr("Unable to read ")
                + filename);
        }
    } else {
        AssetThrowPolicy::WarnOrThrow(kh::tr("No such file: ") + filename);
    }

    if (!result) {
        result = NewInvalid(boundref);
        // leave timestamp alone
        // if it failed but there was a vlid timestamp we want
        // to remember the timestamp of the one that failed
    }

    // store the timestamp so the cache can check it later
    result->timestamp = timestamp;
    result->filesize  = filesize;

    return result;
}

EOF


for (my $i = 0; $i < @ConfigHistory; ++$i) {
    print $fh "extern void FromElement(DOMElement *elem, $ConfigHistory[$i] &self);\n";
}

if (@ConfigHistory > 1) {
    print $fh "#include <khxml/khVersionedDOM.h>\n";
}

print $fh "namespace {\n";
print $fh "  void GetConfig(DOMElement *elem, $config &config) {\n";
if ($missingconfigok) {
    print $fh "    GetElementOrDefault(elem, \"config\", config, $config());\n";
} elsif (@ConfigHistory == 1) {
    print $fh "    GetElement(elem, \"config\", config);\n";
} else {
    my $count = @ConfigHistory;
    print $fh "    Get${count}VersionedElement<";
    for (my $i = 0; $i < @ConfigHistory; ++$i) {
        if ($i != 0) {
            print $fh "                        ";
        }
        print $fh "$ConfigHistory[$i]";
        if ($i == $#ConfigHistory) {
            print $fh "\n";
        } else {
            print $fh ",\n";
        }
    }
    print $fh "                       >(elem, \"config\", config);\n";
}
print $fh "  }\n";
print $fh "} // anonymous namespace\n\n";

    
close($fh);
chmod 0444, $public_cpp;