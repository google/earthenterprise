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

RPMBUILD := $(shell which rpmbuild 2>/dev/null)
ifeq ($(RPMBUILD),)
  RPMBUILD = rpm
endif

REL_DATE := $(shell date +%Y%m%d 2>/dev/null)

# Make all will clean the old rpms to start.
# Note if ~/.rpmmacros changes the packages location, the "rm" line should be changed
# as well.
all rpms:
	rm -rf /home/khbuild/packages/RPMS/*
	$(RPMBUILD) --rcfile /usr/lib/rpm/rpmrc:./.rpmrc --define 'rel_date $(REL_DATE)' -bb GoogleEarthEnterprise.spec
