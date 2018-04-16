---
layout: blogpost
title:  "Open GEE moves to modern C++"
section_id: blog
date:   2018-04-23 13:30:00
author: Open GEE Development Team
---

Hello Everyone!

The Open GEE Development Team would like to make a special announcement about an
effort to upgrade our usage of `C++` to a more modern standard. As of release
`5.2.3`, we will begin compiling Open GEE code using the `C++11` standard. We are
asking that for the duration of `5.2.3` development code still remains compilable
and functioning as expected using the older `C++98` standard. However, once `5.2.3`
is branched then any new code pushed into master and any other future branches
is free to use the `C++11` syntax without this restriction and we will no longer
support compiling Open GEE `C++` code using the `C++98` standard.

We wanted to establish this transition period to be sure there were no major
side effects from changing to the new `C++11` compilation flag. In some cases we
had to add `C++11` syntax but we were careful to make sure that the code still
compiled and functioned as expected using `C++98` standards.

The Open GEE Development Team is committed to keeping the Open GEE source code
relevant and updated. We will continue to make future enhancements to bring the
source code up to more modern standards and technologies.

-Open GEE Development Team
