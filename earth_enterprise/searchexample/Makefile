# Copyright 2008 Google Inc. All Rights Reserved.

RPMBUILD := $(shell which rpmbuild 2>/dev/null)
ifeq ($(RPMBUILD),)
  RPMBUILD = rpm
endif

REL_DATE := $(shell date +%Y%m%d 2>/dev/null)

GoogleEarthSearchExample:
	$(RPMBUILD) --rcfile /usr/lib/rpm/rpmrc:./.rpmrc --define 'rel_date $(REL_DATE)' -bb GoogleEarthSearchExample.spec
