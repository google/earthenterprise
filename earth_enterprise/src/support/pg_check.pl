#!/usr/bin/perl
#
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

# This script finds out the number of rows in each table of GEE specific
# databases and prints a report.
#
# Need to run this script as super user to be successful.
#

$ge_pg_user = "gepguser";

$use_su = `su $ge_pg_user -c "echo -n 1" 2>/dev/null || echo -n 0`;

sub run_as_user {
  my ($user, $cmd) = @_;
  if ($use_su == 1) {
    return `cd /\; su $user -c '$cmd'`;
  } else {
    return `cd /\; sudo -u $user $cmd`;
  }
}


foreach $db ("geplaces", "gepoi", "gesearch", "gestream", "searchexample") {
  print $db . "\n";
  # The following sudo is to run sql "\d" and get all table names in a db
  foreach $table (run_as_user($ge_pg_user, '/opt/google/bin/psql -t -U geuser '
      . $db . ' -c "\\d" 2>/dev/null | cut -f2 -d"|" | cut -f2 -d" "')) {
    chomp $table;
    print "  $table ";
    # The following is to run sql "select count(*) from table_name" and get number of rows
    $num_rows = run_as_user($ge_pg_user, '/opt/google/bin/psql -t -U geuser '
      . $db . ' -c "select count(*) from ' . $table .
      '" 2>/dev/null | tr -s "\n" | tr -s " " | cut -f2 -d" "');
    chomp($num_rows);
    print "$num_rows\n";
  }
}
