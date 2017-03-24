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

# To run: sudo bash dump_pg_tables.sh
# This part of code is to backup data in postgres db in case of a version change
VARPGDIR=/var/opt/google/pgsql
VARPGDATADIR="$VARPGDIR/data"
OLD_VERSION=`cat $VARPGDATADIR/PG_VERSION | cut -c1-3`
NEW_VERSION="8.4"
if [ "$OLD_VERSION" != "$NEW_VERSION" ]
then
  echo "Updating version $OLD_VERSION"
  GEPGUSER=`grep 'GEPGUSER="' /etc/init.d/geserver | cut -f2 -d'"'`

  use_su=`su $GEPGUSER -c "echo -n 1" 2>/dev/null || echo -n 0`

  run_as_user() {
    if [ $use_su -eq 1 ] ; then
      ( cd / ;su $1 -c "bash $2" )
    else
      ( cd / ;sudo -u $1 bash "$2" )
    fi
  }

  PG_DUMP_FILE="/opt/google/bin/pg_dump.sh"

  cat > $PG_DUMP_FILE << "END_CAT"
set -x
set -e
BINDIR=/opt/google/bin
VARPGDIR=/var/opt/google/pgsql
VARPGDATADIR="$VARPGDIR/data"
$BINDIR/pg_ctl -D $VARPGDATADIR -l $VARPGDIR/logs/pg.log start -w
for DB in `$BINDIR/psql -U geuser -l 2>/dev/null | cut -s -f1 -d"|" | grep -v "\<Name\>"`; do
  echo $DB
  # Final pipe to echo prevents failure when no results.
  case $DB in
  gepoi)
    TABLES=`$BINDIR/psql -t -U geuser $DB -c "\\d" 2>/dev/null | \
            cut -s -f2 -d"|" | cut -f2 -d" " | \
            grep -E -v "\<geography_columns\>|\<geometry_columns\>|\<spatial_ref_sys\>" || true` ;;
  searchexample)
    TABLES=`$BINDIR/psql -t -U geuser $DB -c "\\d" 2>/dev/null | \
            cut -s -f2 -d"|" | cut -f2 -d" " | \
            grep -E -v "\<geometry_columns\>|\<san_francisco_neighborhoods\>|\<san_francisco_neighborhoods_gid_seq\>|\<spatial_ref_sys\>" || true ` ;;
  geplaces)
    TABLES=`$BINDIR/psql -t -U geuser $DB -c "\\d" 2>/dev/null | \
            cut -s -f2 -d"|" | cut -f2 -d" " | \
            grep -E -v "\<admincodes\>|\<cities\>|\<countries\>|\<countryinfo\>|\<countrypolys\>|\<countrypolys_gid_seq\>|\<geometry_columns\>|\<geonames\>|\<spatial_ref_sys\>"  || true ` ;;
  template0|template1)
    TABLES=;;
  *)
    TABLES=" " ;;
  esac
  if [ "$TABLES" = "" ]; then
    touch $VARPGDATADIR/$DB.dump
    continue
  fi
  rm -f $VARPGDATADIR/$DB.dump
  if [ "$TABLES" = " " ]; then
    echo "$BINDIR/pg_dump -U geuser --inserts $DB >> $VARPGDATADIR/$DB.dump"
    $BINDIR/pg_dump -U geuser --inserts $DB >> $VARPGDATADIR/$DB.dump
    continue
  fi
  for TABLE in $TABLES; do
    echo "$BINDIR/pg_dump -U geuser --inserts $DB -t $TABLE >> $VARPGDATADIR/$DB.dump"
    $BINDIR/pg_dump -U geuser --inserts $DB -t $TABLE >> $VARPGDATADIR/$DB.dump
  done
done
$BINDIR/pg_ctl -D $VARPGDATADIR -l $VARPGDIR/logs/pg.log stop
END_CAT

  chmod +x "$PG_DUMP_FILE"
  run_as_user $GEPGUSER "$PG_DUMP_FILE"
fi

