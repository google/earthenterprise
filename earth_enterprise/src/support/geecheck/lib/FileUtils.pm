#!/usr/bin/perl -w
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
#
# Some basic file utilities:
# FileContains

package FileUtils;

use strict;
use warnings;

# module constructor
BEGIN {
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # set the version for version checking
    $VERSION     = 1.00;
    @ISA         = qw(Exporter);
    @EXPORT      = qw(&file_contains
                      &get_xml_element
                      &get_xml_element_from_line
                      &get_nth_word
                      &get_nth_word_from_grep
                      &parse_csv_file);
    %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],

    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK   = qw();
}
our @EXPORT_OK;

#------------------------------------------------------------------------------
# Utilities (not exported)
#------------------------------------------------------------------------------
# Strip trailing whitespace, tabs and newlines.
sub strip($) {
  my $input_string = shift @_;
  $input_string =~ s/^\s+//g;
  $input_string =~ s/\t//g;
  $input_string =~ s/\n//g;
  return $input_string;
}

#------------------------------------------------------------------------------
# Utilities (exported)
#------------------------------------------------------------------------------

# Return 1 if the pattern is found in the file
# filename
# pattern: a regexp

sub file_contains($$) {
  my ($filename, $pattern) = @_;
  open(FILE, $filename) or return 0;

  # Loop through until the pattern is found.
  while (<FILE>) {
    if (/$pattern/) {
      close(FILE);
      return 1;
    }
  }
  close(FILE);
  return 0;
}

# Get the specified xml element from a text file.
# If the routine fails, it returns an error message.
# Otherwise it returns an empty message.
# returns (elementvalue, message);
sub get_xml_element($$) {
  my $filename = shift @_;
  my $element = shift @_;

  my $xml_data = readpipe "cat $filename";
  # Look for line of the following form:
  # <element>  value </element>
  $xml_data =~ m/<$element>(.*?)<\/$element>/;
  if (defined $1) {
    my $value = strip($1);
    return ($value, "");
  }
  return (0, "Error: $element not found in $filename");
}

# Parse an XML line which may contain:
# 1) a single element <element>value</element> : returns ("element", value)
# 2) a single element begin: <element> : returns ("element", "")
# 3) a single element end: </element> : returns ("end:element", "")
sub get_xml_element_from_line($) {
  my $line = shift @_;
  if ($line =~ m/<(.*?)>(.*?)<\/(.*?)>/) {
    return ($1, $2);
  }
  if ($line =~ m/<\/(.*?)>/) {
    return ("end:" . $1, "");
  }
  if ($line =~ m/<(.*?)>/) {
    return ($1, "");
  }
  return ("", "");
}

# Get the nth word (whitespace separated) from a file.
# from the first line of a file.
# Takes 2 args: ($filename, $n)
# where n is 0-indexed.
# Return 0 if the word is not found.
sub get_nth_word($$) {
  my ($filename, $n) = @_;
  open(FILE, $filename) or return 0;
  while(<FILE>) {
    my @parts = split(/[ \t]+/, $_);
    close(FILE);
    return 0 if (@parts <= $n);
    my $result = $parts[$n];
    chomp $result;
    return $result;
  }
  close(FILE);
  return 0; # Should never get here.
}

# Get the nth word (delimiter separated) from a file.
# from the first line of a file.
# Takes 4 args: ($filename, $grep_token, $delimiter, $n)
# where n is 0-indexed.
# Return 0 if the word is not found.
sub get_nth_word_from_grep($$$$) {
  my ($filename, $grep_token, $delimiter, $n) = @_;
  open(FILE, "grep $grep_token $filename |") or return 0;
  while(<FILE>) {
    my @parts = split(/$delimiter/, $_);
    close(FILE);
    return 0 if (@parts <= $n);
    my $result = $parts[$n];
    chomp $result;
    return $result;
  }
  close(FILE);
  return 0; # Should never get here.
}

# Parse the CSV file and return a list of arrays for each line.
sub parse_csv_file($) {
  my $filename = shift @_;

  my @csv_array = ();
  # Open the file for reading.
  open(FILE, $filename) or die "Can't open $filename: $!\n";

  # Parse the CSV file and return a list of arrays for each line.
  # Ignore empty lines or "# Comment" lines.
  while(<FILE>) {
    # Strip trailing whitespace, tabs and newlines.
    s/^\s+//g;
    s/\t//g;
    s/\n//g;
    my $line = $_;
    # Ignore empty lines or "# Comment" lines.
    if (!/^$/ && !/^#/) {
      # Split the CSV line.
      my @csv_line = split(/, /, $line);
      push @csv_array, \@csv_line
    }
  }
  close(FILE);
  return @csv_array;
}
END { }       # module clean-up code here (global destructor)

1;  # don't forget to return a true value from the file


