# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line.
SPHINXOPTS    = -W
SPHINXBUILD   = sphinx-build
SPHINXPROJ    = GoogleEarthEnterpriseDocumentation
SOURCEDIR     = .
BUILDDIR      = build
SPHINX2GITHUB = ./sphinxtogithub.py
GETOPENGEEVER = ../../../earth_enterprise/src/scons/getversion.py

OPENGEEVER := $(shell $(GETOPENGEEVER))

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	rm -rf $(BUILDDIR)
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	@$(SPHINX2GITHUB) "$(BUILDDIR)/html"
	rm -rf ../$(OPENGEEVER)
	mkdir ../$(OPENGEEVER)
	cp -r ./$(BUILDDIR)/html/* ../$(OPENGEEVER)
