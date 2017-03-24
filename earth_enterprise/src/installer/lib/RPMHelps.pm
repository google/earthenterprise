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
#

package RPMHelps;

use strict;
use warnings;
use RPMVersion;
use File::Basename;


# ****************************************************************************
# ***  Query RPM DB
# ****************************************************************************
my %RPMVersionCache;
my $InstallPlatform = "";

sub SetPlatform {
    $InstallPlatform = $_[0];
}

sub ClearInstalledVersionCache {
    %RPMVersionCache = ();
}

sub GetInstalledVersionFromCache {
    
    my ($rpmname) = @_;
    if (exists $RPMVersionCache{$rpmname}) {
        return $RPMVersionCache{$rpmname};
    }
    if (exists $RPMVersionCache{lc($rpmname)}) {
        return $RPMVersionCache{lc($rpmname)};
    }

    if ($InstallPlatform eq "ubuntu") {
        $rpmname = lc($rpmname);
	my $cmd = "dpkg-query -W --showformat='\${package}-\${version}:\${architecture}:\${status}\\n' $rpmname |";
	open(RPM, $cmd);
    } else {
	open(RPM, "rpm -q --queryformat \"%{NAME}-%{VERSION}-%{RELEASE}:%{ARCH}\\n\" $rpmname 2>/dev/null |");
    } 

    my $ret = <RPM>;
    close(RPM);
    if (defined $ret) {
	chomp($ret);
	if (($ret eq '') || ($ret =~ /not.installed/)) {
	    $ret = undef;
	}
    }

    my $ref;
    if (defined $ret) {
        my ($fullver, $arch) = split(/:/, $ret);
        $ref = new RPMVersion($fullver);
        $ref->{arch} = $arch;
    }

    $RPMVersionCache{$rpmname} = $ref; 
    return $ref;
}


sub InstalledVersionRef
{
    my ($rpmname) = @_;
    return GetInstalledVersionFromCache($rpmname);
}

sub InstalledVersion
{
    my ($rpmname) = @_;
    my $verref = InstalledVersionRef($rpmname);
    if (defined $verref) {
        return $verref->{fullver};
    }
    return undef;
}

sub IsInstalled
{
    my ($rpmname) = @_;
    my $ref = InstalledVersionRef($rpmname);
    return (defined $ref);
}



# ****************************************************************************
# ***  Package discovery
# ****************************************************************************
sub ReadRPMsFromDirs {
    my $hash = {};
    my $ext = ".rpm";
    
    if ($InstallPlatform eq "ubuntu") {
	$ext = ".deb";
    }

    foreach my $dirname (@_) {
        print "Reading directory: $dirname\n";
        my $arch = basename($dirname);
        print "arch = $arch\n";
	opendir(DIR, $dirname) || die "\nUnable to open $dirname: $!\n";
	foreach my $file (map("$dirname/$_", grep(/$ext$/, readdir(DIR)))) {

            my $fullver = basename $file;
            my $arch;
            if ($ext eq ".deb") {
              $fullver =~ s/_([^\.]+)$ext$//;
              $arch = $1;
            } else {
              $fullver =~ s/\.([^\.]+)$ext$//;
              $arch = $1;
            }
	    my $new = new RPMVersion($fullver);
	    $new->{file} = $file;
	    $new->{arch} = $arch;
            $hash->{$new->{name}} = $new;
            print "  Found file: $file, arch: $arch\n";
	}
	closedir(DIR);
    }

    return $hash;
}

sub FindInstalledRPMByPattern {
    my $pattern = shift;
    my @results;

    if ($InstallPlatform eq "ubuntu") {
	open(RPM, "dpkg-query -W --showformat='\${package}-\${version}\\n' -qa 2>/dev/null |") ||
	    die "Unable to query installed DEBs\n";
    } else {
	open(RPM, "rpm -qa 2>/dev/null |") ||
	    die "Unable to query installed RPMs\n";
    }
    while (<RPM>) {
        chomp;
        push @results, new RPMVersion($_) if (/$pattern/);
    }
    close(RPM);

    return @results;
}

sub FindInstalledFileByPattern {
    my ($rpm, $pattern) = @_;
    my $rpmname = (ref($rpm) eq 'RPMVersion') ? $rpm->{fullname} : $rpm;
    my @results;

    if ($InstallPlatform eq "ubuntu") {
	open(RPM, "dpkg-query -L $rpmname 2>/dev/null |") ||
	    die "Unable to query installed DEBs\n";
    } else {
	open(RPM, "rpm -ql $rpmname 2>/dev/null |") ||
	    die "Unable to query installed RPMs\n";	
    }

    while (<RPM>) {
        chomp;
        push @results, $_ if (/$pattern/);
    }
    close(RPM);

    return @results;
}



# ****************************************************************************
# ***  RPM list management
# ****************************************************************************
sub PruneUpToDate {
    my @rpms;
    foreach my $rpm (@_) {
        my $instver = InstalledVersionRef($rpm->{name});
        if (!(defined($instver) &&
              (RPMVersion::Compare($rpm, $instver) == 0))) {
            push @rpms, $rpm;
	}
    }
    return @rpms;
}

sub PruneNotInstalled {
    my @rpms;
    foreach my $rpm (@_) {
	if (IsInstalled($rpm->{name})) {
            push @rpms, $rpm;
	}
    }
    return @rpms;
}



my %NormalizedDep;
sub NormalizeDep {
    my $dep = shift;
    if (!exists $NormalizedDep{$dep}) {
	my $pack = `rpm -q --whatprovides $dep 2>&1`;
	if ($pack =~ /no package provides/) {
	    $NormalizedDep{$dep} = $dep;
	} else {
	    $pack =~ s/-[^\-]+-[^\-]+$//;
	    $NormalizedDep{$dep} = $pack;
	}
    }
    return $NormalizedDep{$dep};
}

sub UpgradeRPMS {
    my ($rpmsref, $verbosity) = @_;

    print "\nPreparing to install/upgrade RPMs ...\n";
    my @rpms = PruneUpToDate(@{$rpmsref});

    # now get a list of filenames
    my @ToInstall = map($_->{file}, @rpms);

    if (!@ToInstall) {
        print "Nothing to install ...\n";
	return;
    }
    
    my %conflicts;
    my %deps;
    my $cmd;

    if ($InstallPlatform eq "ubuntu") {
	# seems there is no test option of dpkg so we just go ahead and install everything
	$cmd = "dpkg -i @ToInstall";
    } else
    {
	$cmd = "rpm -U --test @ToInstall";
    }

    if ($verbosity > 1) {
        print "----- BEGIN COMMAND -----\n";
        print "$cmd\n";
    } elsif ($verbosity > 0) {
        print "$cmd\n";
    }
    open(RPM, "$cmd 2>&1 |") || die "Unable to invoke rpm\n";
    while (<RPM>) {
        if ($verbosity > 1) {
            print $_;
        }
        chomp;
        if (/\s*(\w+)\s+conflicts\s+with\s+([^-]+)/) {
            push @{$conflicts{$1}}, $2;
        } elsif (/\s*(\S+).*is needed by\s+(.+)/) {
            my $dep = $1;
            my $needee = $2;
            $dep = NormalizeDep($dep);

            # trim leading '(installed)', etc.
            $needee =~ s/^\s*\([^\)]\)\s+//;

            push @{$deps{$dep}}, $needee;
        }
    }
    my $status = close(RPM);
    if ($verbosity > 1) {
        print "----- END COMMAND -----\n";
    }


    if (keys %conflicts) {
        warn "\nConflicts found with installed rpms:\n";
        foreach my $key (sort keys %conflicts) {
            warn "   $key conflicts with " . join(',', @{$conflicts{$key}}) . "\n";
        }
        die "\n";
    }

    if (keys %deps) {
        warn "\nUnresolved dependencies:\n";
        foreach my $dep (sort keys %deps) {
            warn "   $dep needed by " . join(',', @{$deps{$dep}}) . "\n";
            warn "   Please install $dep\n";
        }
        die "\n";
    }


    if (!$status) {
        warn "\nRPM error I'm not equiped to handle: $status\n";
	if ($InstallPlatform ne "ubuntu") {
            warn "rpm -U --test @ToInstall\n";
	    system("rpm -U --test @ToInstall");
	} else {
            warn "dpkg -i @ToInstall\n";
	    system("dpkg -i @ToInstall");
	}
        die "\n";
    }


    print "rpm -U @ToInstall\n";
    if ($InstallPlatform ne "ubuntu" && system("rpm -U @ToInstall") != 0) {
        die "Unable to install RPMs\n";
    }
    ClearInstalledVersionCache();
}




sub EraseRPMS {
    my ($rpmsref, $verbosity) = @_;

    print "\nPreparing to uninstall RPMs ...\n";
    my @rpms = PruneNotInstalled(@{$rpmsref});

    # get a list of rpm names
    my @ToUninstall = map($_->{name}, @rpms);

    while (@ToUninstall) {
	my %needed;
	my $cmd;

	if ($InstallPlatform eq "ubuntu") {
	    $cmd = "dpkg --purge @ToUninstall";
	} else
	{
	    $cmd = "rpm -e --test @ToUninstall";
	}
	if ($verbosity > 1) {
	    print "----- BEGIN COMMAND -----\n";
	    print "$cmd\n";
	} elsif ($verbosity > 0) {
	    print "$cmd\n";
	}
	open(RPM, "$cmd 2>&1 |") || die "Unable to invoke rpm\n";
	while (<RPM>) {
	    if ($verbosity > 1) {
		print $_;
	    }
	    chomp;
	    if (/\s*(\S+).*is needed by/) {
		my $dep = $1;
		$dep = NormalizeDep($dep);

		$needed{$dep} = 1;
	    }
	}
	my $status = close(RPM);
	if ($verbosity > 1) {
	    print "----- END COMMAND -----\n";
	}


	my $oldcount = @ToUninstall;
	if (keys %needed) {
	    if ($verbosity > 0) {
		print "Shrinking uninstall list (avoiding broken dependencies)\n";
	    }
	    @ToUninstall = grep(!exists $needed{$_}, @ToUninstall);
	}

	next if ($oldcount != @ToUninstall);

	if (!$status) {
	    warn "RPM error I'm not equiped to handle: $status\n";
    	    if ($InstallPlatform ne "ubuntu") {
    		warn "rpm -e --test @ToUninstall\n";
    		system("rpm -e --test @ToUninstall");
	    } else {
    		warn "dpkg --purge @ToUninstall\n";
    		system("dpkg --purge @ToUninstall");
	    }
	    die "\n";
	}

	last;
    }

    if (@ToUninstall && $InstallPlatform ne "ubuntu") {
        print "rpm -e @ToUninstall\n";
        if (system("rpm -e @ToUninstall") != 0) {
            die "Unable to uninstall RPMs\n";
        }
        ClearInstalledVersionCache();
    }
}



1;
