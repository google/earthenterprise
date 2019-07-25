---
layout: blogpost
title:  "Git version update to support multibranch builds"
section_id: blog
date:   2018-10-23 13:30:00
author: Open GEE Development Team
---

<br />

Hello Everyone!

The Open GEE Development Team would like to make a special announcement about
the version of Git required to build the project. As of release `Open GEE 5.2.4`,
we will require Git version 1.8.4 or later.

Open GEE uses Git tags to mark the commit corresponding to each release.  The
tags are added at the end of a release cycle.  When you build Open GEE, the
version you are currently building is calclated to be one version beyond the
latest tag in the Git history.  The `git describe` command is used to determine
the latest tag information.

We found that the `git describe` command was not reliably returning the expected
tag when working on different branches. To calculate the version correctly on both
a release branch and the master branch, the `--first-parent` parameter of the
`git describe` command is required.

The `--first-parent` parameter was introduced in Git version 1.8.4.  The requirement
for the Open GEE project has been Git 1.7.1+.  Builds are possible with Git versions
1.7.1 thru 1.8.3, but the version will sometimes be calculated incorrectly.  

Starting today, a warning will be output when building Open GEE with versions of Git less than
1.8.4.  It is strongly encouraged but not yet required to use Git 1.8.4+.

Would you like to be part of the project? Please join us on [Slack](http://slack.opengee.org/) and visit [the project’s Github repository](https://github.com/google/earthenterprise). We would love to see you there!
 
–Open GEE Development Team
