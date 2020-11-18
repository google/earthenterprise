#!/usr/bin/perl -w-
#
# Copyright 2017 Google Inc.
# Copyright 2020 The Open GEE Contributors
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
#

# Turning strict off since it no longer compiles under Perl 5.10.
# This version of Perl is now standard with Lucid Goobuntu.
# use strict;
use Getopt::Long;
use FindBin;
use lib $FindBin::Bin;
#use KeyholeConfigFile;
use File::Basename;
use Cwd qw( realpath );
use IO::Handle;
use Carp;

my $indent = '    ';

sub usage
{
    die "usage: $FindBin::Script [options] --hfile <outname> <keyhole IDL file>\n".
	"       $FindBin::Script [options] --cppfile <outname> <keyhole IDL file>\n".
	"       $FindBin::Script [options] --javafile <outname> <keyhole IDL file>\n".
	"       $FindBin::Script --schemafile <outname> <keyhole IDL file>\n".
	"       $FindBin::Script --help\n".
	"\n".
	"Where [options] can include the following:\n".
        "--read=[SAX|DOM] method to be used for reading (default: DOM)\n".
        "--write=DOM      method to be used for writing (default/only: DOM)\n".
        "\n"
}

my %AtomicTypes = (
		   'bool'      => 1,
		   'float'     => 1,
		   'double'    => 1,
		   'int8'      => 1,
		   'int16'     => 1,
		   'int32'     => 1,
		   'int64'     => 1,
		   'uint8'     => 1,
		   'uint16'    => 1,
		   'uint32'    => 1,
		   'uint64'    => 1,
		   'uchar'     => 1,
		   'uint'      => 1,
		   'ushort'    => 1,
		   'time_t'    => 1,
				   );
foreach my $sign ('signed ', 'unsigned ', '') {
    foreach my $base ('char', 'short', 'short int', 'int',
		      'long', 'long int', 'long long') {
	$AtomicTypes{"$sign$base"} = 1;
    }
}


##########################################
########## Process command line ##########
##########################################
my $thiscommand = "@ARGV";
my $help = 0;
my $numCommands = 0;
my $hfile;
my $impl_hfile;
my $cppfile;
my $javafile;
my $schemafile;
my $readMethod  = 'DOM';
my $writeMethod = 'DOM';
my $idlfile;
GetOptions("hfile=s"       => sub {$hfile=$_[1]; ++$numCommands},
           "cppfile=s"     => sub {$cppfile=$_[1]; ++$numCommands},
           "javafile=s"     => sub {$javafile=$_[1]; ++$numCommands},
           "impl_hfile=s"  => sub {$impl_hfile=$_[1]; ++$numCommands},
           "schemafile=s"  => sub {$schemafile=$_[1]; ++$numCommands},
           "read=s"        => sub { ($readMethod=$_[1]) =~ tr/a-z/A-Z/;
				    die "Invalid 'read' argument\n" unless
					$readMethod =~ /^(SAX|DOM)$/;
				},
           "write=s"       => sub { ($writeMethod=$_[1]) =~ tr/a-z/A-Z/;
				    die "Invalid 'write' argument\n" unless
					$writeMethod =~ /^DOM$/;
				},
           "help|?"     => \$help,
           ) || usage();
usage() if $help;
if ($numCommands != 1) {
    warn "Exactly one of --hfile, --cppfile, --javafile or --schemafile must be specified\n";
    usage();
}
$idlfile = shift;
if (!defined $idlfile) {
    warn "No IDL input file specified\n";
    usage();
}


########################################
########## Parse the IDL file ##########
########################################
my @classes;
my %classes;
my @includes;
my $hquote = '';
my $cppquote = '';
my $IDLFILE;
my $needqt = 0;
my %ExternalHasDeprecated;
my %RequiresGetHeapUsage;
open($IDLFILE, $idlfile) || die "Unable to open $idlfile: $!\n";
my $line;
eval {
    while ($line = GetContentLine($IDLFILE)) {
	if ($line =~ /^\s*class\s+/) {
	    my $class = ParseClass($line, $IDLFILE);
	    push @classes, $class;
	    $classes{$class->{qualname}} = $class;
	} elsif ($line =~ /^\s*\#include/) {
	    push @includes, $line;
	    $needqt = 1 if ($line =~ /qstring/);
	} elsif ($line =~ /^\s*\#hquote\s*$/) {
	    $line = GetLine($IDLFILE);
	    while (defined($line) && ($line !~ /^\s*\#\/hquote\s*$/)) {
		$hquote .= $line . "\n";
		$line = GetLine($IDLFILE);
	    }
	} elsif ($line =~ /^\s*\#cppquote\s*$/) {
	    $line = GetLine($IDLFILE);
	    while (defined($line) && ($line !~ /^\s*\#\/cppquote\s*$/)) {
		$cppquote .= $line . "\n";
		$line = GetLine($IDLFILE);
	    }
    } elsif ($line =~ /^ExternalHasDeprecated:\s*(\w+)/) {
        $ExternalHasDeprecated{$1} = 1;
    } elsif ($line =~ /^\s*\#requiresgetheapusage\s*$/) {
        $RequiresGetHeapUsage = 1;
        push @includes, "#include \"CacheSizeCalculations.h\"";
    } else {
	    die "Expected class definition found '$line'\n";
	}
    }
};
if ($@) {
    my $lastline = $IDLFILE->input_line_number();
    die "$idlfile:$lastline: $@";
}
my $lastline = $IDLFILE->input_line_number();
die "$idlfile:$lastline: No class definitions found\n" unless @classes;
close($IDLFILE);


############################################
########## Write the hfile output ##########
############################################
if (defined $hfile) {
    EnsureDirExists($hfile);
    my $fh;
    open($fh, '>', $hfile) || die "Unable to open $hfile: $!\n";

    my $hprot = basename($hfile);
    $hprot =~ tr/./_/;

    EmitAutoGeneratedWarning($fh);
    EmitCopyright($fh);
    print $fh <<EOH;
#ifndef __$hprot
#define __$hprot

EOH
    foreach my $include (@includes) {
	print $fh $include, "\n"
    }
    print $fh "\n" if (@includes);
    if (length($hquote)) {
        print $fh $hquote;
        print $fh "\n";
    }
    foreach my $class (@classes) {
	DumpClass($class, $fh);
	DumpGlobalClassHelpers($class, $fh);
        print $fh "\n\n";
    }
    print $fh <<EOH;

#endif /* __$hprot */
EOH
    close($fh);
    chmod 0444, $hfile;
}


#################################################
########## Write the impl_hfile output ##########
#################################################
if (defined $impl_hfile) {
    EnsureDirExists($impl_hfile);
    my $fh;
    open($fh, '>', $impl_hfile) || die "Unable to open $impl_hfile: $!\n";

    my $hprot = basename($impl_hfile);
    $hprot =~ tr/./_/;

    my $hfile = basename($impl_hfile);
    $hfile =~ s/[\.-_]impl\.h$/\.h/i;

    EmitAutoGeneratedWarning($fh);
    EmitCopyright($fh);
    print $fh <<EOH;
#ifndef __$hprot
#define __$hprot

EOH
    # include some basic xml headers
    print $fh "#include <cstdint>\n";
    print $fh "#include <khxml/khxml.h>\n";
    if ($writeMethod eq 'DOM' || $readMethod eq 'DOM') {
        print $fh "#include <xercesc/dom/DOM.hpp>\n";
    }

    # include my class definition
    print $fh "#include \"$hfile\"\n";
    print $fh "\n";

    # emit my externs
    foreach my $class (@classes) {
        EmitDOMWriterExterns($class, $fh) if ($writeMethod eq 'DOM');
        EmitDOMReaderExterns($class, $fh) if ($readMethod eq 'DOM');
    }
    print $fh "\n";

    # only now can we include khdom.h
    # this way the template methods (for vectors, sets, etc) will see my
    # specializations for ToElement, FromElement, etc.
    if ($writeMethod eq 'SAX' || $readMethod eq 'SAX') {
	print $fh "#include <khxml/khsax.h>\n";
    }
    if ($writeMethod eq 'DOM' || $readMethod eq 'DOM') {
	print $fh "#include <khxml/khdom.h>\n";
    }
    print $fh "#include <khxml/IsUpToDate.h>\n";
    print $fh "\n";

    print $fh <<EOH;

#endif /* __$hprot */
EOH
    close($fh);
    chmod 0444, $impl_hfile;
}



##############################################
########## Write the cppfile output ##########
##############################################
if (defined $cppfile) {
    EnsureDirExists($cppfile);
    my $fh;
    open($fh, '>', $cppfile) || die "Unable to open $cppfile: $!\n";
    my $impl_hfile = basename($cppfile);
    $impl_hfile =~ s/\.[^\.]+$/\_impl.h/;

    EmitAutoGeneratedWarning($fh);
    EmitCopyright($fh);

    print $fh "#include <khGuard.h>\n";
    print $fh "#include \"$impl_hfile\"\n";
    print $fh "\n";
    print $fh "using namespace khxml;\n\n";

    if (length($cppquote)) {
        print $fh $cppquote;
        print $fh "\n";
    }
    foreach my $class (@classes) {
	if ($class->{cppquote}) {
	    print $fh $class->{cppquote};
	}

	EmitDOMWriter($class, $fh) if ($writeMethod eq 'DOM');
	EmitSAXReader($class, $fh) if ($readMethod eq 'SAX');
	EmitDOMReader($class, $fh) if ($readMethod eq 'DOM');

    }
    close($fh);
    chmod 0444, $cppfile;
}


###########################################
########## Write the java output ##########
###########################################
if (defined $javafile) {
    #EnsureDirExists($javafile);

    my $fh;
    foreach my $class (@classes) {
      open($fh, '>', $javafile."/".$class->{name}.".java") || die "Unable to open $javafile: $!\n";

    #EmitAutoGeneratedWarning($fh);
    #EmitCopyright($fh);
    print $fh <<EOH;
package com.google.client.config;

import com.google.gwt.user.client.rpc.IsSerializable;
import java.util.Vector;
import java.util.Set;

EOH
	JavaDumpClass($class, $fh);
	#DumpGlobalClassHelpers($class, $fh);
        #print $fh "\n\n";
        close($fh);
    }

    #chmod 0444, $javafile;
}

#################################################
########## Write the schemafile output ##########
#################################################
if (defined $schemafile) {
}



########## helper functions ##########
sub GetLine
{
    my $fh = shift;
    my $line = <$fh>;
    return undef unless defined($line);
    chomp($line);
    while ($line =~ s/\\$//) {
	my $add = <$fh>;
	return $line unless defined($add);
	chomp($add);
	$line .= $add;
    }
    return $line;
}

sub GetLineSansComments
{
    my $fh = shift;
    my $line = GetLine($fh);
    if (defined($line)) {
	$line =~ s,//.*$,,;		# strip c++ comments
    }
    return $line;
}

sub GetContentLine
{
    my $fh = shift;
    my $line;
    do {
	$line = GetLineSansComments($fh);
    } while (defined($line) && $line =~ /^\s*$/);
    return $line;
}

our ($deprecated, $isuptodateignore, $isuptodateignoreif, $ignoreif,
     $xmlattr, $xmlbody, $xmlchild, $xmlskiptag);

sub ParseClass
{
    my ($line, $fh, $subclassof) = @_;

    my ($namespace, $name, $base, $open) =
	$line =~ /^\s*class\s+((?:\w+::)*)(\w+)\s*(?:\:\s*(\w+)\s*)?(?:(\{)\s*)?$/;
    if (length($namespace) == 0) {
        undef $namespace;
    } else {
        $namespace =~ s/::$//;
    }

    die "Expected 'class NAME': found '$line'\n"
	unless defined($name);

    if (!defined($open)) {
	$line = GetContentLine($fh);
	if ($line !~ /^\s*\{\s*$/) {
	    die "Expected '{': found '$line'\n";
	}
    }

    if (defined($base) && !exists($classes{$base})) {
	die "Unknown base class: $base\n";
    }

    if (defined($subclassof) && defined($namespace)) {
        die "Cannot use namespace syntax with sub classes\n";
    }

    my $class = {
        namespace  => $namespace,
	name       => $name,
	base       => $base ? $classes{$base} : undef,
	qualname   => (defined($subclassof) ? "${subclassof}::${name}" :
                       (defined($namespace) ? "${namespace}::${name}" :
                        $name)),
	members    => [],
	depmembers => [],
	subclasses => [],
	enums      => [],
	typedefs   => [],
	subclassof => defined($namespace) ? $namespace : $subclassof,
	hquote     => '',
	cppquote   => '',
        idl_version => undef,
    };

    my %pragmas = ( LoadAndSave => 1,
		    StrLoadAndSave => 1,
		    DontEmitDefaults => 1,
		    TagName => 1,
		    GenerateOperatorLessThan => 1,
		    NoConstructors => 1,
		    NoVoidConstructor => 1,
		    AfterLoad => 1,
		    DontEmitFunc => 1,
		    GenerateIsUpToDate => 1,
		    GenerateDebugIsUpToDate => 1,
                    ExtractOfClass => 1,
		    );

    $line = GetContentLine($fh);
    while ($line !~ /^\s*\};?\s*$/) {
	$line =~ s/^\s+//;
	$line =~ s/\s+$//;
	if ($line =~ /\s*class\s+/) {
	    push @{$class->{subclasses}}, ParseClass($line, $fh, $class->{qualname});
	} elsif ($line =~ /^\s*enum\s+/) {
	    push @{$class->{enums}}, ParseEnum($line, $fh, $class->{qualname});
	} elsif ($line =~ /^\s*typedef\s+/) {
	    ParseTypedef($class, $line, $fh);
	} elsif ($line =~ /^\s*\#pragma\s+(\w+)\s*(\w+)?$/) {
	    if (exists $pragmas{$1}) {
		$class->{$1} = defined($2) ? $2 : 1;
	    } else {
		die "Unrecognized pragma: $1\nValid pragmas are: ".
		    join(', ', sort keys %pragmas)."\n";
	    }
	} elsif ($line =~ /^\s*\#version\s+(\d+)$/) {
          push @{$class->{members}}, {
            name    	=> "idl_version",
            tagname 	=> "idl_version",
            type    	=> "int",
            newdefault    => $1,
            saveoverride  => $1,
            loaddefault   => "0",
            xmlattr     => 1,
            mutable     => 1,
          };
          $class->{idl_version} = $1;
	} elsif ($line =~ /^\s*\#hquote\s*$/) {
	    $line = GetLine($fh);
	    while (defined($line) && ($line !~ /^\s*\#\/hquote\s*$/)) {
		$class->{hquote} .= $line . "\n";
		$line = GetLine($fh);
	    }
	} elsif ($line =~ /^\s*\#cppquote\s*$/) {
	    $line = GetLine($fh);
	    while (defined($line) && ($line !~ /^\s*\#\/cppquote\s*$/)) {
		$class->{cppquote} .= $line . "\n";
		$line = GetLine($fh);
	    }
	} elsif ($line =~ /^\s*$name\(\s*([^\)]+)\)/) {
	    my $args = $1;
	    $args =~ s/\s+$//;
	    my @args = split(/\s*,\s+/, $args);
	    my @argnames = map {/(\w+)(?:\s*=\s*(?:.*))?$/} @args;
	    $class->{constructor} = {
		content  => $line,
		args     => [ @args ],
		argnames => [ @argnames ],
	    }
	} else {
	    $line =~ s/;$//;	# strip optional semicolon from end of line
            local ($deprecated, $isuptodateignore, $isuptodateignoreif,
                   $ignoreif,
                   $xmlattr, $xmlbody, $xmlchild, $xmlskiptag);
            my $found = 1;
            while ($found) {
                no strict "refs";
                $found = 0;
                foreach my $word ('deprecated', 'isuptodateignore',
                                  'isuptodateignoreif(\S+)',
                                  'ignoreif(\S+)',
                                  'xmlattr', 'xmlbody', 'xmlchild(\w+)',
                                  'xmlskiptag') {
                    my $key;
                    my $exp;
                    my $haveval;
                    if ($word =~ /^(\w+)\((.*)\)$/) {
                        $key = $1;
                        $exp = "$1\\(($2)\\)\\s+";
                        $haveval = 1;
                    } else {
                        $key = $word;
                        $exp = "$word\\s+";
                        $haveval = 0;
                    }
                    if ($line =~ s/^\s*$exp//i) {
                        $found = 1;
                        if ($haveval) {
                            $$key = $1;
                        } else {
                            $$key = 1;
                        }
                    }
                }
            }
	    my ($type, $mname, $default) =
		$line =~ /^\s*(\w+[^=]*)\s+(\w+)\s*(?:=\s*(.+)\s*)?$/;

	    die "Expected member definition: found '$line'\n"
		unless defined($type) && defined($mname);

	    # trim any whitspace off of end of type
	    $type =~ s/\s+$//;

	    # check for split defaults
	    # =new:0L, load:-1L
	    # =load:-1L, new:0L
	    my $newdefault = $default;
	    my $loaddefault = $default;
	    if (defined $default) {
		if ($default =~ /^new:\s*(.+),\s*load:\s*(.+)/) {
		    $newdefault = $1;
		    $loaddefault = $2;
		} elsif ($default =~ /^load:\s*(.+),\s*new:\s*(.+)/) {
		    $loaddefault = $1;
		    $newdefault = $2;
		} elsif ($default =~ /^load:\s*(.+)/) {
		    $loaddefault = $1;
		    undef $newdefault;
		}
		# trim any trailing whitespace
		if (defined $newdefault) {
                    $newdefault =~ s/\s+$//;
                }
		if (defined $loaddefault) {
                    $loaddefault =~ s/\s+$//;
                }
	    }

            {
                my @exclusive = ('xmlattr', 'xmlbody', 'xmlchild');
                my $count = 0;
                foreach my $key (@exclusive) {
                    ++$count if defined($$key);
                }
                if ($count > 1) {
                    warn "Bad modfiers for $class->{qualname}::$mname\n";
                    die "The following modifiers are mutually exclusive: " . join(", ", @exclusive) . "\n";
                }
            }
            if (defined($xmlskiptag) &&
                (defined($xmlattr) || defined($xmlbody))) {
                warn "Bad modfiers for $class->{qualname}::$mname\n";
                die "xmlskiptag cannot be specified with xmlattr or xmlbody\n";
            }

	    my $tagname = $mname;
	    $tagname =~ s/_$//;
	    if ($deprecated) {
		if (! defined $default) {
		    die "Deprecated field $class->{qualname}::$mname must have a default value\n";
		}
		if ($isuptodateignore || defined($isuptodateignoreif) || defined($ignoreif)) {
		    die "Deprecated field $class->{qualname}::$mname also flagged as ignore\n";
		}
		push @{$class->{depmembers}}, {
		    name    	=> $mname,
		    tagname 	=> $tagname,
		    type    	=> $type,
		    newdefault  => $newdefault,
		    loaddefault => $loaddefault,
                    xmlattr     => $xmlattr,
                    xmlbody     => $xmlbody,
                    xmlchild    => $xmlchild,
                    xmlskiptag  => $xmlskiptag,
		};
	    } else {
		push @{$class->{members}}, {
		    name    	=> $mname,
		    tagname 	=> $tagname,
		    type    	=> $type,
		    newdefault  => $newdefault,
		    loaddefault => $loaddefault,
		    isuptodateignore => $isuptodateignore,
		    isuptodateignoreif => $isuptodateignoreif,
		    ignoreif    => $ignoreif,
                    xmlattr     => $xmlattr,
                    xmlbody     => $xmlbody,
                    xmlchild    => $xmlchild,
                    xmlskiptag  => $xmlskiptag,
		};
	    }
	}

	$line = GetContentLine($fh);
    }
#    die "class '$name' has no members\n" unless defined($class->{base}) || @{$class->{members}};

    if (!defined $class->{subclassof} || defined($class->{namespace})) {
        GatherMemberTypes($class);
#	$class->{externtypes} = [ GatherExternTypes($class) ];
    }

    if (!exists $class->{TagName}) {
	$class->{TagName} = $class->{name};
    }

    if (defined $class->{GenerateDebugIsUpToDate}) {
        $class->{GenerateIsUpToDate} = 1;
    }

    return $class;
}

sub EmitMemberWriter
{
    my ($member, $fh, $prefix) = @_;
    my $mname  = $member->{name};
    my $mtname = $member->{tagname};

    if (defined $member->{saveoverride}) {
      print $fh $prefix, "$member->{type} ${mname}_save_override = $member->{saveoverride};\n";
      $mname = "${mname}_save_override";
    }

    if ($member->{xmlattr}) {
        print $fh $prefix, "AddAttribute(dom_elem__, \"$mtname\", $mname);\n";
    } elsif ($member->{xmlbody}) {
        print $fh $prefix, "ToElement(dom_elem__, $mname);\n";
    } elsif (defined($member->{xmlchild}) && $member->{xmlskiptag}) {
        print $fh $prefix, "ToElementWithChildName(dom_elem__, \"$member->{xmlchild}\",  $mname);\n";
    } elsif (defined($member->{xmlchild})) {
        print $fh $prefix, "(void)AddElementWithChildName(dom_elem__, \"$mtname\", \"$member->{xmlchild}\", $mname);\n";
    } elsif ($member->{xmlskiptag}) {
        print $fh $prefix, "ToElement(dom_elem__, $mname);\n";
    } else {
        print $fh $prefix, "AddElement(dom_elem__, \"$mtname\", $mname);\n";
    }

}

sub EmitMemberReader
{
    my ($member, $fh, $prefix, $obj) = @_;
    my $mname  = $member->{name};
    my $mtname = $member->{tagname};

    if (defined($member->{loaddefault})) {
        if ($member->{xmlattr}) {
            print $fh $prefix, "GetAttributeOrDefault(elem, \"$mtname\", $obj.$mname, $member->{loaddefault});\n";
        } elsif ($member->{xmlbody}) {
            print $fh $prefix, "GetElementBody(elem, $obj.$mname);\n";
        } elsif (defined($member->{xmlchild}) && $member->{xmlskiptag}) {
            print $fh $prefix, "FromElementWithChildName(elem, \"$member->{xmlchild}\", $obj.$mname);\n";
        } elsif (defined($member->{xmlchild})) {
            print $fh $prefix, "GetElementWithChildNameOrDefault(elem, \"$mtname\", \"$member->{xmlchild}\", $obj.$mname, $member->{loaddefault});\n";
        } elsif ($member->{xmlskiptag}) {
            print $fh $prefix, "FromElement(elem, $obj.$mname);\n";
        } else {
            my $temp=$member->{loaddefault};
            if (index($temp, "Qt::") != -1)
            {
               $temp = "static_cast<QColor>(".$temp.")";
            }
            print $fh $prefix, "GetElementOrDefault(elem, \"$mtname\", $obj.$mname, $temp);\n";
        }
    } else {
        if ($member->{xmlattr}) {
            print $fh $prefix, "GetAttribute(elem, \"$mtname\", $obj.$mname);\n";
        } elsif ($member->{xmlbody}) {
            print $fh $prefix, "GetElementBody(elem, $obj.$mname);\n";
        } elsif ($member->{xmlchild} && $member->{xmlskiptag}) {
            print $fh $prefix, "FromElementWithChildName(elem, \"$member->{xmlchild}\", $obj.$mname);\n";
        } elsif ($member->{xmlchild}) {
            print $fh $prefix, "GetElementWithChildName(elem, \"$mtname\", \"$member->{xmlchild}\", $obj.$mname);\n";
        } elsif ($member->{xmlskiptag}) {
            print $fh $prefix, "FromElement(elem, $obj.$mname);\n";
        } else {
            print $fh $prefix, "GetElement(elem, \"$mtname\", $obj.$mname);\n";
        }
    }

}


sub GatherMemberTypes
{
    my ($class) = @_;
    my $types = {};
    my %localnames;
    my %typedefs;
    foreach my $subclass (@{$class->{subclasses}}) {
	grep($types->{$_} = 1, keys %{GatherMemberTypes($subclass)});
	$localnames{$subclass->{name}} = $subclass->{qualname};
    }
    foreach my $enum (@{$class->{enums}}) {
	$localnames{$enum->{name}} = $enum->{qualname};
    }
    foreach my $typedef (@{$class->{typedefs}}) {
	$localnames{$typedef->{name}} = $typedef->{qualname};
	$typedefs{$typedef->{qualname}} = $typedef->{type};
    }
    foreach my $member (@{$class->{members}}) {
	if (exists $localnames{$member->{type}}) {
	    $member->{qualtype} = $localnames{$member->{type}};
	} else {
	    $member->{qualtype} = $member->{type};
	}
	$types->{$member->{qualtype}} = 1;
    }
    foreach my $member (@{$class->{depmembers}}) {
	if (exists $localnames{$member->{type}}) {
	    $member->{qualtype} = $localnames{$member->{type}};
	} else {
	    $member->{qualtype} = $member->{type};
	}
	$types->{$member->{qualtype}} = 1;
    }

    # extract template arguments
    my $numExtracted;
    do {
	$numExtracted = 0;
	foreach my $type (keys %{$types}) {
	    if (exists $typedefs{$type}) {
		$type = $typedefs{$type};
	    }
	    if ($type =~ /^[^<]+<(.+)>[^>]*$/) {
		my $args = $1;
		$args =~ s/^\s+//;
		$args =~ s/\s+$//;
		foreach my $arg ( split(/\s*,\s*/, $args) ) {
		    if (exists $localnames{$arg}) {
			$arg = $localnames{$arg};
		    }
		    if ( ! exists $types->{$arg} ) {
			++$numExtracted;
			$types->{$arg} = 1;
		    }
		}
	    }
	}
    } while ($numExtracted);

    return $types;
}

# sub GatherExternTypes
# {
#     my ($class) = @_;
#     my $types = GatherMemberTypes($class);

#     foreach my $type (keys %{$types}) {
#  	# atomic types are already declared
#  	delete $types->{$type},next if exists $AtomicTypes{$type};
#  	# khSize is predefined
#  	delete $types->{$type},next if $type =~ /^khSize/;
#  	# khOffset is predefined
#  	delete $types->{$type},next if $type =~ /^khOffset/;
#  	# khExtents is predefined
#  	delete $types->{$type},next if $type =~ /^khExtents/;
#  	# khMetaData is predefined
#  	delete $types->{$type},next if $type =~ /^khMetaData/;
#  	# stuff in the khTypes namesapce are predefined
#  	delete $types->{$type},next if $type =~ /^khTypes::/;
#  	# supported STL types are already declared
#  	delete $types->{$type},next if $type =~ /^std::/;
#  	# QString is predefined
#  	delete $types->{$type},next if $type =~ /^QString/;
#  	# QColor is predefined
#  	delete $types->{$type},next if $type =~ /^QColor/;
#  	# Defaultable is predefined
#  	delete $types->{$type},next if $type =~ /^Defaultable/;
#  	# we're about to define all the ones that belong to us
#  	delete $types->{$type},next if $type =~ /^$class->{name}::/;

#  	# maybe somebody else in this file has already defined it?
#  	if ($type =~ /^(.*)::/) {
#  	    my $classname = $1;
#  	    if (exists $classes{$classname}) {
#  		delete $types->{$type};
#  		next;
#  	    }
#  	}

#  	# maybe it _is_ somebody else in this file
#  	delete $types->{$type},next if exists($classes{$type});

#     }

#     return sort keys %{$types};
# }


sub ParseEnum
{
    my ($line, $fh, $parentclass) = @_;

    my ($name, $enums) =
	$line =~ /^\s*enum\s+(\w+)\s*\{\s*([^\}]+)\s*\};?\s*$/;
    die "Syntax error parsing enum\n" unless
	defined($name) && defined($enums);
    $enums =~ s/\s+$//;
    my @enums = map {
	$_ =~ /^\s*(\w+)\s*(?:=\s*(\S+)\s*)?$/;
	{ name => $1,
	  value  => $2 };
    } split(/\s*,\s*/, $enums);

    die "enum '$name' has no enumerators\n" unless @enums;

    return { name     => $name,
	     qualbase => "${parentclass}",
	     qualname => "${parentclass}::${name}",
	     enumerators => [@enums],
	 };
}

sub ParseTypedef
{
    my ($class, $line) = @_;
    my $parentname = $class->{qualname};

    my ($type, $name) =
	$line =~ /^\s*typedef\s+(.*)\b(\w+)\s*;\s*$/;
    die "Unable to interpret typedef. Only simple typedefs supported\n" unless
	defined($name) && defined($type);
    $type =~ s/\s+$//;

    push @{$class->{typedefs}}, {
	name     => $name,
	qualbase => "${parentname}",
	qualname => "${parentname}::${name}",
	type     => $type,
    };
}

sub EmitProtection($$$$)
{
    my ($prot, $protState, $fh, $pad) = @_;

    if (${$protState} ne $prot) {
	print $fh $pad, "$prot:\n";
	${$protState} = $prot;
    }
}

sub HasDeprecatedMembers
{
    my $type = shift;
    confess "INTERNAL ERROR bad value to HasDeprecatedMembers\n" unless defined $type;
    if (ref($type) ne 'HASH') {
        if (exists $ExternalHasDeprecated{$type}) {
            return 1;
        }
        # type is not a class hash - see if it's a name of one
	return 0 unless exists $classes{$type};
	$type = $classes{$type};
    }
    if (exists $type->{hasDeprecated}) {
	return $type->{hasDeprecated};
    }
    if (defined($type->{base})&&HasDeprecatedMembers($type->{base}{qualname})){
	$type->{hasDeprecated} = 1;
    } elsif (@{$type->{depmembers}}) {
	$type->{hasDeprecated} = 1;
    } else {
	$type->{hasDeprecated} = 0;
	foreach my $member (@{$type->{members}}) {
	    if (HasDeprecatedMembers($member->{qualtype})) {
		$type->{hasDeprecated} = 1;
		last;
	    }
	}
    }
    return $type->{hasDeprecated};
}




sub DumpClass
{
    my ($class, $fh, $pad) = @_;
    $pad = '' unless defined($pad);
    my $prot = 'private';

    if (defined($class->{namespace})) {
        print $fh $pad, "namespace $class->{namespace} {\n\n";
    }

    print $fh $pad, "class $class->{name}";
    print $fh " : public $class->{base}{qualname}" if ($class->{base});
    print $fh " {\n";
    foreach my $enum (@{$class->{enums}}) {
	EmitProtection('public', \$prot, $fh, $pad);
	print $fh $pad, $indent, "enum $enum->{name} { ",
	join(', ',
	     map(defined($_->{value})? "$_->{name} = $_->{value}" : $_->{name},
			 @{$enum->{enumerators}})
	     ), " };\n";
    }
    foreach my $subclass (@{$class->{subclasses}}) {
	EmitProtection('public', \$prot, $fh, $pad);
	DumpClass($subclass, $fh, $pad.$indent);
    }
    foreach my $typedef (@{$class->{typedefs}}) {
	EmitProtection('public', \$prot, $fh, $pad);
	print $fh $pad, $indent, "typedef $typedef->{type} $typedef->{name};\n";
    }

    # members
    EmitProtection('public', \$prot, $fh, $pad);
    foreach my $member (@{$class->{members}}) {
      if ($member->{mutable}) {
	print $fh $pad, $indent, "mutable $member->{type} $member->{name};\n";
      } else {
	print $fh $pad, $indent, "$member->{type} $member->{name};\n";
      }
    }

    if (HasDeprecatedMembers($class)) {
	print $fh "\n";
	print $fh $pad, $indent, "class DeprecatedMembers";
	if (defined($class->{base}) &&
	    HasDeprecatedMembers($class->{base}{qualtype})) {
	    print $fh " : public $class->{base}{qualname}::DeprecatedMembers {\n";
	} else {
	    print $fh " {\n";
	}

	print $fh $pad, $indent, "public:\n";

	foreach my $member (@{$class->{depmembers}}) {
	    print $fh $pad, $indent, $indent, "$member->{type} $member->{name};\n";
	    if (HasDeprecatedMembers($member->{qualtype})) {
		print $fh $pad, $indent, $indent, "$member->{type}::DeprecatedMembers $member->{name}_depmembers;\n";
	    }
	}

	foreach my $member (@{$class->{members}}) {
	    if (HasDeprecatedMembers($member->{qualtype})) {
		print $fh $pad, $indent, $indent, "$member->{type}::DeprecatedMembers $member->{name}_depmembers;\n";
	    }
	}
	print $fh $pad, $indent, "};\n";
    }


    # constructors
    print $fh "\n";
    EmitProtection('public', \$prot, $fh, $pad);
    my $numMembers = @{$class->{members}};
    my $numDefaultMembers = grep(defined($_->{newdefault}), @{$class->{members}});
    my $i;

    if (!defined $class->{NoConstructors}) {
	if (!defined $class->{NoVoidConstructor}) {
	    # constructor with no args (don't emit if user constructor has no args)
	    # don't emit if the user told us not to
	    if (($numMembers != $numDefaultMembers) &&
		(!exists $class->{constructor} || @{$class->{constructor}{args}})) {
		print $fh $pad, $indent, "$class->{name}(void)";
		my $numdefault = 0;
		for ($i = 0; $i < $numMembers; ++$i) {
		    my $member = $class->{members}[$i];
		    if (defined($member->{newdefault})) {
			if ($numdefault++ == 0) {
			    print $fh " :\n";
			} else {
			    print $fh ",\n";
			}
			print $fh $pad, $indent, $indent, "$member->{name}($member->{newdefault})";
		    }
		}
		print $fh " { }\n";
	    }
	}

	if (exists $class->{constructor}) {
	    print $fh $pad, $indent, $class->{constructor}{content}, "\n";
	} elsif ($numMembers || (defined $class->{base} &&
				 exists($class->{base}{constructor}))) {
	    # constructor with args for all members
	    print $fh $pad, $indent, "$class->{name}(";
	    my $namelen = length($class->{name});
	    my $needpad = 0;

	    # args needed for base class constructor
	    if (defined $class->{base} && exists($class->{base}{constructor})) {
		my $baseargs = $class->{base}{constructor}{args};
		my $i = 0;
		foreach my $basearg (@{$baseargs}) {
		    print $fh $pad, $indent, ' 'x($namelen+1) if $needpad;
		    $needpad = 1;
		    print $fh "$basearg";
		    print $fh ",\n" if $numMembers || $i < @{$baseargs}-1;
		    ++$i;
		}
	    }

	    # args for my members
	    for ($i = 0; $i < $numMembers; ++$i) {
		my $member = $class->{members}[$i];
		print $fh $pad, $indent, ' 'x($namelen+1) if $needpad;;
		$needpad = 1;
		if (exists $AtomicTypes{$member->{type}}) {
		    print $fh "$member->{type} $member->{name}_";
		} else {
		    print $fh "const $member->{type} &$member->{name}_";
		}
		if (defined($member->{newdefault})) {
		    print $fh " = ", $member->{newdefault},
		}
		print $fh ",\n" unless $i == $numMembers-1;
	    }
	    print $fh ") :\n";

	    # invoke user supplied base class contructor
	    if (defined $class->{base} && exists($class->{base}{constructor})) {
		print $fh $pad, $indent, $indent, "$class->{base}->{qualname}(",
		join(', ', @{$class->{base}{constructor}{argnames}}), ")";
		print $fh ",\n" if $numMembers;
	    }

	    for ($i = 0; $i < $numMembers; ++$i) {
		my $member = $class->{members}[$i];
		print $fh $pad, $indent, $indent, "$member->{name}($member->{name}_)";
		print $fh ",\n" unless $i == $numMembers-1;
	    }
	    print $fh " { }\n";
	}
    }

    # comparison operator
    {
	print $fh $pad, $indent, "bool operator== (const $class->{name} &o) const\n";
	print $fh $pad, $indent, "{\n";
	my $haveFirst = 0;

	if (defined $class->{base}) {
	    print $fh $pad, $indent x2, "return $class->{base}{qualname}::operator==(o)";
	    $haveFirst = 1;
	}

	foreach my $member (@{$class->{members}}) {
	    if ($haveFirst) {
		print $fh " &&\n", $pad, $indent x2, "       ";
	    } else {
		print $fh $pad, $indent x2, "return ";
		$haveFirst = 1;
	    }
	    print $fh "$member->{name} == o.$member->{name}";
	}
	if (!$haveFirst) {
	    print $fh $pad, $indent x2, "return true";
	}
	print $fh ";\n";
	print $fh $pad, $indent, "}\n";

	print $fh $pad, $indent, "bool operator!= (const $class->{name} &o) const\n";
	print $fh $pad, $indent, "{\n";
	print $fh $pad, $indent x2, "return ! operator==(o);\n";
	print $fh $pad, $indent, "}\n";
    }

    # less than operator
    if (defined $class->{GenerateOperatorLessThan}) {
	print $fh $pad, $indent, "bool operator< (const $class->{name} &o) const\n";
	print $fh $pad, $indent, "{\n";
	my $haveFirst = 0;

	if (defined $class->{base}) {
	    print $fh $pad, $indent x2, "return $class->{base}{qualname}::operator<(o)";
	    $haveFirst = 1;
	}

	foreach my $member (@{$class->{members}}) {
	    if ($haveFirst) {
		print $fh " ||\n", $pad, $indent x2, "       ";
	    } else {
		print $fh $pad, $indent x2, "return ";
		$haveFirst = 1;
	    }
	    print $fh "$member->{name} < o.$member->{name}";
	}
	if (!$haveFirst) {
	    print $fh $pad, $indent x2, "return true";
	}
	print $fh ";\n";
	print $fh $pad, $indent, "}\n";
    }

    # IsUpToDate
    if (defined $class->{GenerateIsUpToDate}) {
	print $fh $pad, $indent, "bool IsUpToDate(const $class->{name} &o) const;\n";
    }


    # ExtractOfClass
    if (defined $class->{ExtractOfClass}) {
        my $FullClass = $classes{$class->{ExtractOfClass}};
        print $fh "\n";
        print $fh $pad, $indent, "// ExtractOfClass $FullClass->{qualname}\n";
	print $fh $pad, $indent, "explicit $class->{name}(const $FullClass->{qualname} &o);\n";
        print $fh "\n";
    }

    # LoadAndSave
    if (defined $class->{LoadAndSave}) {
	print $fh "\n";
	print $fh $pad, $indent, "bool Load(const std::string &file) throw();\n";
	print $fh $pad, $indent, "bool Save(const std::string &file) const throw();\n";
    }

    # StrLoadAndSave
    if (defined $class->{StrLoadAndSave}) {
	print $fh $pad, $indent, "bool LoadFromString(const std::string &buf, const std::string &ref) throw();\n";
	print $fh $pad, $indent, "bool SaveToString(std::string &buf, const std::string &ref) const throw();\n";
    }

    if (defined $class->{idl_version}) {
      print $fh $pad, $indent, "bool _IsCurrentIdlVersion(void) const throw() {\n";
      print $fh $pad, $indent, $indent, "return $class->{idl_version} == idl_version;\n";
      print $fh $pad, $indent, "}\n";
      print $fh $pad, $indent, "void _UpdateIdlVersion(void) const throw() {\n";
      print $fh $pad, $indent, $indent, "idl_version = $class->{idl_version};\n";
      print $fh $pad, $indent, "}\n";
    } else {
      print $fh $pad, $indent, "bool _IsCurrentIdlVersion(void) const throw() {\n";
      print $fh $pad, $indent, $indent, "return true;\n";
      print $fh $pad, $indent, "}\n";
      print $fh $pad, $indent, "void _UpdateIdlVersion(void) const throw() {\n";
      print $fh $pad, $indent, "}\n";
    }

    # GetHeapUsage
    if ($RequiresGetHeapUsage) {
        my $haveFirst = 0;
        my $curr = 1;
        my $size = @{$class->{members}};
        print $fh $pad, $indent, "std::uint64_t GetHeapUsage() const {\n";
        foreach my $member (@{$class->{members}}) {
            if ($curr == $size) {
                if ($haveFirst) {
                    print $fh $pad, $indent x2, "+ ::GetHeapUsage($member->{name});\n";
                } else {
                    print $fh $pad, $indent x2, "return ::GetHeapUsage($member->{name});\n";
                }
                last;
            }
	        if ($haveFirst) {
                print $fh $pad, $indent x2, "+ ::GetHeapUsage($member->{name})\n";
	        } else {
	    	    print $fh $pad, $indent, $indent, "return ::GetHeapUsage($member->{name})\n";
	    	    $haveFirst = 1;
	        }
            $curr++;
	    }
        print $fh $pad, $indent, "}\n";
    }

    if ($class->{hquote}) {
	print $fh $class->{hquote};
    }

    print $fh $pad, "};\n\n";


    if (defined($class->{namespace})) {
        print $fh $pad, "}  // namespace $class->{namespace}\n\n";
    }

}

sub JavaSubType
{
  my ($in_type, $class) = @_;

  my $out_type = $in_type;
  my $out_comment;
  my $out_warn;

  # strip Defaultable<>
  if ($in_type =~ m/^Defaultable</) {
    $out_type =~ /Defaultable<\s*(.*)\s*>/;
    $out_type = $1;
  }

  if ($out_type eq "std::string" || $out_type eq "QString") {
    $out_type = "String";
  } elsif ($out_type eq "uint") {
    $out_type = "int";
  } elsif ($out_type eq "bool") {
    $out_type = "boolean";
  } elsif ($out_type =~ m/^std::vector/) {
    $out_type = "Vector";
    $out_warn = "true";
  } elsif ($out_type =~ m/^std::set/) {
    $out_type = "Set";
    $out_warn = "true";
  } elsif ($out_type =~ m/^std::map/) {
    $out_type = "Map";
    $out_warn = "true";
  } else {
    # compare all enums
    foreach my $enum (@{$class->{enums}}) {
      if ($out_type eq $enum->{name}) {
        $out_type = "int";
        $out_comment = "// was enum $in_type";
      }
    }
  }

  if (!defined($out_comment)) {
    if ($in_type ne $out_type) {
      $out_comment = "// was $in_type";
    } else {
      $out_comment = "";
    }
  }

  return ($out_type, $out_comment, $out_warn);
}

sub JavaDefaultVal
{
  my ($in_val, $class) = @_;

  foreach my $enum (@{$class->{enums}}) {
    foreach my $enums (@{$enum->{enumerators}}) {
      if ($in_val eq $class->{name}."::".$enums->{name}) {
        return ("$enum->{name}_$enums->{name}");
      }
    }
  }

  return ($in_val);
}

sub JavaDumpClass
{
    my ($class, $fh, $pad) = @_;
    $pad = '' unless defined($pad);
    my $prot = 'private';

    print "Java class = $class->{name}\n";

    print $fh $pad, "public class $class->{name} implements IsSerializable {\n";
    foreach my $enum (@{$class->{enums}}) {
	print $fh $pad, $indent, "// enum $enum->{name}\n";
        my $enum_val = 0;
        foreach my $enums (@{$enum->{enumerators}}) {
          print $fh $pad, $indent, "public static final int $enum->{name}_$enums->{name} = $enum_val;\n";
          ++$enum_val;
        }
        print $fh "\n";
    }

    foreach my $member (@{$class->{members}}) {
        print "  member = $member->{name}\n";
        my ($type, $comment, $warn) = JavaSubType($member->{type}, $class);
        if (defined $warn) {
          print $fh $pad, $indent, "\@SuppressWarnings(\"unchecked\")\n";
        }
        if (defined $member->{newdefault}) {
          my $default = JavaDefaultVal($member->{newdefault}, $class);
          print $fh $pad, $indent, "public $type $member->{name} = $default;\n";
        } else {
          print $fh $pad, $indent, "public $type $member->{name};\n";
        }
    }

    # constructors
    print $fh "\n";

    my $numMembers = @{$class->{members}};
    my $numDefaultMembers = grep(defined($_->{newdefault}), @{$class->{members}});
    my $i;

    if (!defined $class->{NoConstructors}) {
	if (!defined $class->{NoVoidConstructor}) {
	    # constructor with no args (don't emit if user constructor has no args)
	    # don't emit if the user told us not to
	    if (($numMembers != $numDefaultMembers) &&
		(!exists $class->{constructor} || @{$class->{constructor}{args}})) {
		print $fh $pad, $indent, "public $class->{name}() {\n";
		for ($i = 0; $i < $numMembers; ++$i) {
		    my $member = $class->{members}[$i];
		    if (defined($member->{newdefault})) {
			print $fh $pad, $indent, $indent, "this.$member->{name} = $member->{newdefault};\n";
		    }
		}
		print $fh $pad, $indent, "}\n\n";
	    }
	}

	if (exists $class->{constructor}) {
	    print $fh $pad, $indent, $class->{constructor}{content}, "\n";
	} elsif ($numMembers || (defined $class->{base} &&
				 exists($class->{base}{constructor}))) {
	    # constructor with args for all members
	    print $fh $pad, $indent, "public $class->{name}(";
	    my $namelen = length($class->{name});
	    my $needpad = 0;

	    # args needed for base class constructor
	    if (defined $class->{base} && exists($class->{base}{constructor})) {
		my $baseargs = $class->{base}{constructor}{args};
		my $i = 0;
		foreach my $basearg (@{$baseargs}) {
		    print $fh $pad, $indent, ' 'x($namelen+1) if $needpad;
		    $needpad = 1;
		    print $fh "$basearg";
		    print $fh ",\n" if $numMembers || $i < @{$baseargs}-1;
		    ++$i;
		}
	    }

	    # args for my members
	    for ($i = 0; $i < $numMembers; ++$i) {
		my $member = $class->{members}[$i];
		print $fh $pad, $indent, ' 'x($namelen+8) if $needpad;;
		$needpad = 1;

                my ($type, $comment) = JavaSubType($member->{type}, $class);
                print $fh "$type $member->{name}";
		print $fh ",\n" unless $i == $numMembers-1;
	    }
	    print $fh ") {\n";

	    # invoke user supplied base class contructor
	    if (defined $class->{base} && exists($class->{base}{constructor})) {
		print $fh $pad, $indent, $indent, "$class->{base}->{qualname}(",
		join(', ', @{$class->{base}{constructor}{argnames}}), ")";
		print $fh ",\n" if $numMembers;
	    }

	    for ($i = 0; $i < $numMembers; ++$i) {
		my $member = $class->{members}[$i];
		print $fh $pad, $indent, $indent, "this.$member->{name} = $member->{name};\n";
	    }
	    print $fh $pad, $indent, "}\n\n";

            print $fh $pad, $indent, "public $class->{name}() {\n";
            print $fh $pad, $indent, "}\n";

	}
    }

    print $fh $pad, "}\n\n";
}

sub DumpGlobalClassHelpers
{
    my ($class, $fh) = @_;

    foreach my $subclass (@{$class->{subclasses}}) {
	DumpGlobalClassHelpers($subclass, $fh);
    }
    foreach my $enum (@{$class->{enums}}) {
        print $fh "extern bool IsUpToDate($enum->{qualname} a, $enum->{qualname} b);\n";
	print $fh "extern std::string ToString($enum->{qualname});\n";
        print $fh "extern void FromString(const std::string &, $enum->{qualname} &);\n";
    }
    if ($class->{GenerateIsUpToDate}) {
        print $fh "inline bool IsUpToDate(const $class->{qualname} &a, const $class->{qualname} &b) {\n";
        print $fh $indent, "return a.IsUpToDate(b);\n";
        print $fh "}\n\n";
    }
    if ($RequiresGetHeapUsage) {
        print $fh "inline std::uint64_t GetHeapUsage(const $class->{qualname} &obj) {\n";
        print $fh $indent, "return obj.GetHeapUsage();\n";
        print $fh "}\n\n";
    }
}


sub EmitDOMWriterExterns
{
    my ($class, $fh) = @_;

    # EmitDOMWriterExterns for my subclasses
    foreach my $subclass (@{$class->{subclasses}}) {
	EmitDOMWriterExterns($subclass, $fh);
    }

    print $fh "extern void ToElement(khxml::DOMElement *elem, const $class->{qualname} &self);\n";

    if (defined $class->{DontEmitFunc}) {
        print $fh "extern void AddElement(khxml::DOMElement *parent, const std::string &tagname, const $class->{qualname} &self);\n";
    }
}


sub EmitDOMWriter
{
    my ($class, $fh) = @_;

    # emit externs for all the ToElement routines that we call but don't define
#     if (exists $class->{externtypes}) {
# 	foreach my $type (@{$class->{externtypes}}) {
# 	    print $fh "extern void ToElement(DOMElement*, const $type &);\n";
# 	}
# 	print $fh "\n";
#     }

    # emit DOMWriters for my subclasses
    foreach my $subclass (@{$class->{subclasses}}) {
	EmitDOMWriter($subclass, $fh);
    }

    # emit DOMWriters for my enums
    foreach my $enum (@{$class->{enums}}) {
	EmitEnumDOMWriter($enum, $fh);
    }

    my $DOM_classname = "$class->{qualname}_DOM";
    $DOM_classname =~ s/::/_/g;


    print $fh "class $DOM_classname : public $class->{qualname} {\n";
    print $fh $indent, "public:\n";
    print $fh $indent, "void PopulateElement(DOMElement *dom_elem__) const;\n";
    print $fh "};\n\n";

    print $fh "void ${DOM_classname}::PopulateElement(DOMElement *dom_elem__) const\n";
    print $fh "{\n";

    # call my base class to have it do its part
    if (defined $class->{base}) {
	print $fh $indent, "ToElement(dom_elem__, static_cast<const $class->{base}{qualname} &>(*this));\n";
    }

    foreach my $member (@{$class->{members}}) {
	my $mname = $member->{name};
        if (defined($member->{ignoreif})) {
	    print $fh $indent, "if (!($member->{ignoreif})) {\n";
            EmitMemberWriter($member, $fh, $indent . $indent);
	    print $fh $indent, "}\n";
	} elsif (exists($class->{DontEmitDefaults}) &&
            defined($member->{loaddefault})) {
	    print $fh $indent, "if ($mname != $member->{loaddefault}) {\n";
            EmitMemberWriter($member, $fh, $indent . $indent);
	    print $fh $indent, "}\n";
	} else {
            EmitMemberWriter($member, $fh, $indent);
	}
    }

    print $fh "}\n\n";


    # globally scoped ToElement helper function
    print $fh "void\n";
    print $fh "ToElement(DOMElement *elem, const $class->{qualname} &self)\n";
    print $fh "{\n";
    print $fh $indent, "reinterpret_cast<const $DOM_classname *>(&self)->PopulateElement(elem);\n";
    print $fh "}\n\n";

    # IsUpToDate
    if (defined $class->{GenerateIsUpToDate} &&
        !defined $class->{GenerateDebugIsUpToDate}) {
	print $fh "bool $class->{qualname}::IsUpToDate(const $class->{qualname} &o) const\n";
	print $fh "{\n";
	my $haveFirst = 0;

        if (defined $class->{DontEmitFunc}) {
	    print $fh $indent, "if (DontEmitFunc() && o.DontEmitFunc()) return true;\n\n";
        }

	if (defined $class->{base}) {
	    print $fh $indent, "return $class->{base}{qualname}::IsUpToDate(o)";
	    $haveFirst = 1;
	}

	foreach my $member (@{$class->{members}}) {
	    next if $member->{isuptodateignore};

	    if ($haveFirst) {
		print $fh " &&\n", $indent, "       ";
	    } else {
		print $fh $indent, "return ";
		$haveFirst = 1;
	    }

	    if (defined($member->{ignoreif})) {
                print $fh "(($member->{ignoreif}) ||\n";
                print $fh $indent, "        ::IsUpToDate($member->{name}, o.$member->{name}))";
            } elsif (defined($member->{isuptodateignoreif})) {
                print $fh "(($member->{isuptodateignoreif}) ||\n";
                print $fh $indent, "        ::IsUpToDate($member->{name}, o.$member->{name}))";
            } else {
                print $fh "::IsUpToDate($member->{name}, o.$member->{name})";
            }
	}
	if (!$haveFirst) {
	    print $fh $indent, "return true";
	}
	print $fh ";\n";
	print $fh "}\n\n";
    }

    # DebugIsUpToDate
    if (defined $class->{GenerateDebugIsUpToDate}) {
	print $fh "bool $class->{qualname}::IsUpToDate(const $class->{qualname} &o) const\n";
	print $fh "{\n";

        if (defined $class->{DontEmitFunc}) {
	    print $fh $indent, "if (DontEmitFunc() && o.DontEmitFunc()) return true;\n\n";
        }

        my $name = $class->{qualname};
	if (defined $class->{base}) {
	    print $fh $indent, "if (!$class->{base}{qualname}::IsUpToDate(o)) {\n";
            print $fh $indent, "  notify(NFY_WARN, \"$name IsUpToDate: base out of date\");\n";
            print $fh $indent, "  return false;\n";

            print $fh $indent, "}\n"
	}

	foreach my $member (@{$class->{members}}) {
	    next if $member->{isuptodateignore};

	    if (defined($member->{ignoreif})) {
                print $fh $indent, "if (!($member->{ignoreif}) &&\n";
                print $fh $indent, "    !::IsUpToDate($member->{name}, o.$member->{name})) {\n";
                print $fh $indent, "  notify(NFY_WARN, \"$name IsUpToDate: $member->{name} out of date\");\n";
                print $fh $indent, "  return false;\n";
                print $fh $indent, "}\n"
            } elsif (defined($member->{isuptodateignoreif})) {
                print $fh $indent, "if (!($member->{isuptodateignoreif}) &&\n";
                print $fh $indent, "    !::IsUpToDate($member->{name}, o.$member->{name})) {\n";
                print $fh $indent, "  notify(NFY_WARN, \"$name IsUpToDate: $member->{name} out of date\");\n";
                print $fh $indent, "  return false;\n";
                print $fh $indent, "}\n"
            } else {
                print $fh $indent, "if (!::IsUpToDate($member->{name}, o.$member->{name})) {\n";
                print $fh $indent, "  notify(NFY_WARN, \"$name IsUpToDate: $member->{name} out of date\");\n";
                print $fh $indent, "  return false;\n";
                print $fh $indent, "}\n"
            }
	}
        print $fh $indent, "return true";
	print $fh ";\n";
	print $fh "}\n\n";
    }

    # ExtractOfClass
    if (defined $class->{ExtractOfClass}) {
        my $FullClass = $classes{$class->{ExtractOfClass}};
        print $fh "\n";
        print $fh "// ExtractOfClass $FullClass->{qualname}\n";

        print $fh "// contruct me from full class\n";
	print $fh "$class->{qualname}::$class->{name}(const $FullClass->{qualname} &o) :\n";
        my $first = 1;
	foreach my $member (@{$class->{members}}) {
            if ($first) {
                $first = 0;
            } else {
                print $fh ",\n";
            }
            print $fh $indent, "$member->{name}(o.$member->{name})";
	}
        print $fh "\n{ }\n\n";
    }


    if (defined $class->{LoadAndSave}) {

	print $fh <<EOF;
bool
$class->{qualname}::Save(const std::string &file) const throw()
{
    auto doc = CreateEmptyDocument("$class->{TagName}");
    if (!doc) {
        notify(NFY_WARN, "Unable to create empty document: $class->{TagName}");
        return false;
    }
    bool status = false;
    try {
        DOMElement *top = doc->getDocumentElement();
        if (top) {
            ToElement(top, *this);
            status = WriteDocument(doc.get(), file);
        } else {
            notify(NFY_WARN, "Unable to create document element %s", file.c_str());
        }
    } catch (const XMLException& toCatch) {
        notify(NFY_WARN, "Error saving %s: %s",
               file.c_str(), XMLString::transcode(toCatch.getMessage()));
    } catch (const DOMException& toCatch) {
        notify(NFY_WARN, "Error saving %s: %s",
               file.c_str(), XMLString::transcode(toCatch.msg));
    } catch (const std::exception &e) {
        notify(NFY_WARN, "Error saving %s: %s", file.c_str(), e.what());
    } catch (...) {
        notify(NFY_WARN, "Unknown problem saving %s", file.c_str());
    }
    return status;
}

EOF
    }

    if (defined $class->{StrLoadAndSave}) {

	print $fh <<EOF;

bool
$class->{qualname}::SaveToString(std::string &buf, const std::string &ref) const throw()
{
    auto doc = CreateEmptyDocument("$class->{TagName}");
    if (!doc) return false;
    bool status = false;
    try {
        DOMElement *top = doc->getDocumentElement();
        if (top) {
            ToElement(top, *this);
            status = WriteDocumentToString(doc.get(), buf);
        } else {
            notify(NFY_WARN, "Unable to create document element %s", ref.c_str());
        }
    } catch (const std::exception &e) {
        notify(NFY_WARN, "Error saving %s: %s", ref.c_str(), e.what());
    } catch (...) {
        notify(NFY_WARN, "Unknown problem saving %s", ref.c_str());
    }
    return status;
}

EOF

    }


    if (defined $class->{DontEmitFunc}) {

	print $fh <<EOF;

void
AddElement(khxml::DOMElement *parent, const std::string &tagname, const $class->{qualname} &self)
{
  if (!self.DontEmitFunc()) {
    khxml::DOMElement* elem =
        parent->getOwnerDocument()->createElement(ToXMLStr(tagname));
    ToElement(elem, self);
    parent->appendChild(elem);
  }
}

EOF

    }

}

sub EmitEnumDOMWriter
{
    my ($enum, $fh) = @_;

    print $fh "bool IsUpToDate($enum->{qualname} a, $enum->{qualname} b)\n";
    print $fh "{\n";
    print $fh $indent, "return a == b;\n";
    print $fh "}\n\n";


    print $fh "std::string\n";
    print $fh "ToString($enum->{qualname} self)\n";
    print $fh "{\n";
    print $fh $indent, "switch (self) {\n";
    foreach my $enumerator (@{$enum->{enumerators}}) {
	print $fh $indent, "case $enum->{qualbase}::$enumerator->{name}:\n";
	print $fh $indent, $indent, "return \"$enumerator->{name}\";\n";
    }
    print $fh $indent, "}\n";
    print $fh $indent, "return std::string();\n";
    print $fh "}\n\n";
}


sub EmitSAXReader
{
    my ($class, $fh) = @_;
}

sub EmitDOMReaderExterns
{
    my ($class, $fh) = @_;

    # EmitDOMReaderExterns for my subclasses
    foreach my $subclass (@{$class->{subclasses}}) {
	EmitDOMReaderExterns($subclass, $fh);
    }

    if (HasDeprecatedMembers($class)) {
	print $fh "extern void FromElementWithDeprecated(khxml::DOMElement *elem, $class->{qualname} &self,\n";
	print $fh "                                      $class->{qualname}::DeprecatedMembers &depmembers);\n";
	print $fh "extern void FromElement(khxml::DOMElement *elem, $class->{qualname} &self);\n";
    } else {
	print $fh "extern void FromElement(khxml::DOMElement *elem, $class->{qualname} &self);\n";
    }
    if (defined $class->{DontEmitFunc}) {
        print $fh "extern void GetElement(khxml::DOMElement *parent, const std::string &tagname, $class->{qualname} &self);\n";
    }
}

sub EmitDOMReader
{
    my ($class, $fh) = @_;

    # emit externs for the FromElement routines that we call but don't define
#     if (exists $class->{externtypes}) {
# 	foreach my $type (@{$class->{externtypes}}) {
# 	    print $fh "extern void FromElement(DOMElement*, $type &);\n";
# 	}
# 	print $fh "\n";
#     }

    # emit DOMReaders for my subclasses
    foreach my $subclass (@{$class->{subclasses}}) {
	EmitDOMReader($subclass, $fh);
    }

    # emit DOMReaders for my enums
    foreach my $enum (@{$class->{enums}}) {
	EmitEnumDOMReader($enum, $fh);
    }

    if (HasDeprecatedMembers($class)) {
	print $fh "void\n";
	print $fh "FromElementWithDeprecated(DOMElement *elem, $class->{qualname} &self,\n";
	print $fh "                          $class->{qualname}::DeprecatedMembers &depmembers)\n";
	print $fh "{\n";
    } else {
	print $fh "void\n";
  print $fh "FromElement(DOMElement *elem, $class->{qualname} &self)\n";
	print $fh "{\n";
    }

    # call my base class to have it do its part
    if (defined $class->{base}) {
	if (HasDeprecatedMembers($class->{base}{qualname})) {
	    print $fh $indent, "FromElementWithDeprecated(elem, static_cast<$class->{base}{qualname} &>(self),\n";
	    print $fh $indent, "                          static_cast<$class->{base}{qualname}::DeprecatedMembers&>(depmembers));\n";
	} else {
	    print $fh $indent, "FromElement(elem, static_cast<$class->{base}{qualname} &>(self));\n";
	}
    }

    foreach my $member (@{$class->{members}}) {
	my $mname = $member->{name};
	my $mtname = $member->{tagname};

        if (HasDeprecatedMembers($member->{qualtype})) {
            if (defined($member->{loaddefault})) {
		print $fh $indent, "GetElementWithDeprecatedOrDefault(elem, \"$mtname\", self.$mname, $member->{loaddefault}, depmembers.${mname}_depmembers);\n";
            } else {
		print $fh $indent, "GetElementWithDeprecated(elem, \"$mtname\", self.$mname, depmembers.${mname}_depmembers);\n";
            }
        } else {
            EmitMemberReader($member, $fh, $indent, 'self');
        }

    }

    if (HasDeprecatedMembers($class)) {
	foreach my $member (@{$class->{depmembers}}) {
	    my $mname = $member->{name};
	    my $mtname = $member->{tagname};
	    if (HasDeprecatedMembers($member->{qualtype})) {
		print $fh $indent, "GetElementWithDeprecatedOrDefault(elem, \"$mtname\", depmembers.$mname, $member->{loaddefault}, depmembers.${mname}_depmembers);\n";
	    } else {
                EmitMemberReader($member, $fh, $indent, 'depmembers');
	    }
	}
    }

    if (exists $class->{AfterLoad}) {
	if (HasDeprecatedMembers($class)) {
	    print $fh $indent, "self.AfterLoad(depmembers);\n";
	} else {
	    print $fh $indent, "self.AfterLoad();\n";
	}
    }
    print $fh "}\n\n";

    if (HasDeprecatedMembers($class)) {
	# little wrapper to avoid all pieces needing to pass depmembers
	print $fh "void\n";
        print $fh "FromElement(DOMElement *elem, $class->{qualname} &self) {\n";
	print $fh $indent, "$class->{qualname}::DeprecatedMembers depmembers;\n";
	print $fh $indent, "FromElementWithDeprecated(elem, self, depmembers);\n";
	print $fh "}\n";
    }


    my $DECLARE_DEPRECATED;
    my $PASS_DEPRECATED;
    my $CALL_FROMELEMENT;
    if (HasDeprecatedMembers($class)) {
	$DECLARE_DEPRECATED = "$class->{name}::DeprecatedMembers depmembers;";
	$PASS_DEPRECATED    = ", depmembers";
        $CALL_FROMELEMENT   = "FromElementWithDeprecated";
    } else {
	$DECLARE_DEPRECATED = "";
	$PASS_DEPRECATED    = "";
        $CALL_FROMELEMENT   = "FromElement";
    }


    # Load Routine
    if (defined $class->{LoadAndSave}) {
	print $fh <<EOF;
bool
$class->{qualname}::Load(const std::string &file) throw()
{
    bool result = false;
    //std::unique_ptr<GEDocument>
    auto doc = ReadDocument(file);
    if (doc) {
        try {
            DOMElement *docelem = doc->getDocumentElement();
            if (docelem) {
                $DECLARE_DEPRECATED
                $CALL_FROMELEMENT(docelem, *this$PASS_DEPRECATED);
                result = true;
            } else {
                notify(NFY_WARN, "No document element loading %s",
                       file.c_str());
            }
        } catch (const std::exception &e) {
            notify(NFY_WARN, "%s while loading %s", e.what(), file.c_str());
        } catch (...) {
            notify(NFY_WARN, "Unable to load %s", file.c_str());
        }
    } else {
        notify(NFY_WARN, "Unable to read %s", file.c_str());
    }
    return result;
}

EOF
    }

    if (defined $class->{StrLoadAndSave}) {
	print $fh <<EOF;

bool
$class->{qualname}::LoadFromString(const std::string &buf,
    const std::string &ref) throw()
{
    bool result = false;
    //std::unique_ptr<GEDocument>
    auto doc = ReadDocumentFromString(buf, ref);
    if (doc) {
        try {
            DOMElement *docelem = doc->getDocumentElement();
            if (docelem) {
                $DECLARE_DEPRECATED
                $CALL_FROMELEMENT(docelem, *this$PASS_DEPRECATED);
                result = true;
            } else {
                notify(NFY_WARN, "No document element loading %s",
                       ref.c_str());
            }
        } catch (const std::exception &e) {
            notify(NFY_WARN, "%s while loading %s", e.what(), ref.c_str());
        } catch (...) {
            notify(NFY_WARN, "Unable to load %s", ref.c_str());
        }
    } else {
        notify(NFY_WARN, "Unable to read %s", ref.c_str());
    }
    return result;
}

EOF
    }

    if (defined $class->{DontEmitFunc}) {
	print $fh <<EOF;

void
GetElement(khxml::DOMElement *parent, const std::string &tagname, $class->{qualname} &self)
{
  GetElementOrDefault(parent, tagname, self, $class->{qualname}());
}


EOF
    }
}

sub EmitEnumDOMReader
{
    my ($enum, $fh) = @_;

    print $fh "void\n";
    print $fh "FromString(const std::string &enumStr, $enum->{qualname} &self)\n";
    print $fh "{\n";
    my $i = 0;
    for ($i = 0; $i < @{$enum->{enumerators}}; ++$i) {
      my $item = $enum->{enumerators}[$i];
      my $conditional = "enumStr == \"$item->{name}\" || enumStr == \"$item->{value}\"";
      if ($i == 0) {
        print $fh $indent, "if ($conditional) {\n";
      } else {
        print $fh $indent, "} else if ($conditional) {\n";
      }
      print $fh $indent, $indent, "self = $enum->{qualbase}::$item->{name};\n";
      print $fh $indent, $indent, "return;\n";
    }
    print $fh $indent, "}\n";
    print $fh $indent, "throw khException(kh::tr(\"Invalid string '%1' for enum '%2'\").arg(enumStr.c_str()).arg(\"$enum->{qualname}\"));\n";
    print $fh "}\n\n";
}


sub GetSAXHandler {
    my ($type) = @_;
    if (exists $classes{$type}) {
	return "${type}SAXHandler";
    } elsif ($type eq 'Asset::MetaData') {
	return 'MetaSAXHandler';
    } elsif ($type =~ /^std::vector\s*<\s*(.*)>$/) {
	my $subtype = $1;
	chomp $subtype;
	my $subhandler = GetSAXHandler($subtype);
	return "ArraySAXHandler($subhandler)";
    } else {
	return 'ScalarSAXHandler';
    }
}

sub GetDOMHandler {
    my ($type) = @_;
    if (exists $classes{$type}) {
	return ("${type}::FromDOMElement", $type);
    } elsif ($type eq 'Asset::MetaData') {
	return 'MetaFromDOMElement';
    } elsif ($type =~ /^std::vector\s*<\s*(.*)>$/) {
	my $subtype = $1;
	chomp $subtype;
	return ("ArrayFromDOMElement", GetDOMHandler($subtype));
    } else {
	return 'ScalarFromDOMElement';
    }
}

sub EmitAutoGeneratedWarning
{
    my ($fh, $cs) = @_;
    $cs = '//' unless defined $cs;
    print $fh <<WARNING;
$cs ***************************************************************************
$cs ***  This file was AUTOGENERATED with the following command:
$cs ***
$cs ***  $FindBin::Script $thiscommand
$cs ***
$cs ***  Any changes made here will be lost.
$cs ***************************************************************************
WARNING
}

sub EmitCopyright
{
    my ($fh) = @_;
    my $year = (localtime(time))[5]+1900;
    print $fh <<COPYRIGHT;
// Copyright $year-$year Google Inc. All Rights Reserved
COPYRIGHT
}

sub EnsureDirExists
{
    my $dir = dirname($_[0]);
    if (! -d $dir) {
	EnsureDirExists($dir);
	mkdir($dir) || die "Unable to mkdir $dir: $!\n";
    }
}
