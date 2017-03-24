#!/bin/sh
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

# Gets file list on standard input and RPM_BUILD_ROOT as first parameter
# and searches for omitted files (not counting directories).
# Returns it's output on standard output.
#
# filon@pld.org.pl

RPM_BUILD_ROOT=$1

if [ ! -d "$RPM_BUILD_ROOT" ] ; then
	cat > /dev/null
	exit 1
fi

[ "$TMPDIR" ] || TMPDIR=/tmp
FILES_DISK=`mktemp $TMPDIR/rpmXXXXXX`
FILES_RPM=`mktemp $TMPDIR/rpmXXXXXX`

find $RPM_BUILD_ROOT -type f | LC_ALL=C sort -u > $FILES_DISK
LC_ALL=C sort -u > $FILES_RPM

for f in `diff "$FILES_DISK" "$FILES_RPM" | grep "^< " | cut -c3-`; do
	test $RPM_BUILD_ROOT/usr/share/info/dir != "$f" && echo $f | sed -e "s#^$RPM_BUILD_ROOT#   #g"
done

rm -f $FILES_DISK
rm -f $FILES_RPM
