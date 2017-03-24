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

package Users;

use strict;
use warnings;

our $FusionUser    = ['gefusionuser', 'gegroup'];
our $ApacheUser    = ['geapacheuser', 'gegroup'];
our $TomcatUser    = ['getomcatuser', 'gegroup'];
our $PostgresUser  = ['gepguser',     'gegroup'];
our $PrefsFile     = "/tmp/fusion.prefs";

sub BEGIN {
  our $PrefsFile     = "/tmp/fusion.prefs";
  if (-e $Users::PrefsFile) {
    my $fh;
    open($fh, "<$Users::PrefsFile") or die "Problem opening $PrefsFile - are you running as root?";
    my $userprefs_tab = {};
    while (my $line = <$fh>) {
      chomp $line;
      my ($varname, $user, $group) = split(":", $line);
      $Users::userprefs_tab{$varname} = [$user, $group];
    }
    if (exists($Users::userprefs_tab{"FusionUser"})) {
      $FusionUser = $Users::userprefs_tab{"FusionUser"};
    }
    if (exists($Users::userprefs_tab{"ApacheUser"})) {
      $ApacheUser = $Users::userprefs_tab{"ApacheUser"};
    }
    if (exists($Users::userprefs_tab{"TomcatUser"})) {
      $TomcatUser = $Users::userprefs_tab{"TomcatUser"};
    }
    if (exists($Users::userprefs_tab{"PostgresUser"})) {
      $PostgresUser = $Users::userprefs_tab{"PostgresUser"};
    }
  }
}

sub SaveUserPrefs {
  my $fh;
  open($fh, ">$Users::PrefsFile") or die "Problem writing $Users::PrefsFile - are you running as root?";
  printf $fh ("FusionUser:" . join(":", @$FusionUser) . "\n");
  printf $fh ("ApacheUser:" . join(":", @$ApacheUser) . "\n");
  printf $fh ("TomcatUser:" . join(":", @$TomcatUser) . "\n");
  printf $fh ("PostgresUser:" . join(":", @$PostgresUser) . "\n");
}

sub GetUserAndGroupId {
    my $user = shift;
    my ($name,$passwd,$uid,$gid,
        $quota,$comment,$gcos,$dir,$shell,$expire) = getpwnam($user);
    if (defined $uid) {
        return ($uid,$gid);
    } else {
        return ();
    }
}

sub GetGroupId {
    my $group = shift;
    return scalar(getgrnam($group));
}

sub GetGroupName {
    my $gid = shift;
    return getgrgid($gid);
}

sub GroupExists
{
    my $group = shift;
    return defined(GetGroupId($group));
}

sub CreateGroup {
    my ($groupname, $gid) = @_;

    my $extra = '';
    if (defined $gid) {
        $extra .= " -g $gid";
    }

    system("/usr/sbin/groupadd $extra $groupname");

    # sometimes the create succeeds, but the check immediately after fails.
    # just wait a little bit
    sleep(2);

    return GroupExists($groupname);
}

sub CreateUser {
    my ($username, $groupname, $uid) = @_;

    my $extra = '';
    if (defined $uid) {
        $extra .= " -u $uid";
    }
    if (-f '/etc/redhat-release') {
        $extra .= " -n";
    }
    my $gid = GetGroupId($groupname);
    if (!defined($gid)) {
        warn "No such group: $groupname\n";
        return 0;
    }

    system("/usr/sbin/useradd $extra -d / -s /bin/false -g $gid $username");

    # sometimes the create succeeds, but the check immediately after fails.
    # just wait a little bit
    sleep(2);

    my ($newuid, $newgid) = GetUserAndGroupId($username);
    return (defined($newuid) &&
            defined($newgid) &&
            ($newgid == $gid));
}

sub FixUserGroup {
    my ($username, $groupname) = @_;

    my $extra = '';
    my $gid = GetGroupId($groupname);
    if (!defined($gid)) {
        warn "No such group: $groupname\n";
        return 0;
    }

    system("/usr/sbin/usermod $extra -g $gid $username");

    # sometimes the create succeeds, but the check immediately after fails.
    # just wait a little bit
    sleep(2);

    my ($newuid, $newgid) = GetUserAndGroupId($username);
    return (defined($newuid) &&
            defined($newgid) &&
            ($newgid == $gid));
}
