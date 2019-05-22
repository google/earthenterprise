---
layout: blogpost
title:  "Open GEE moves to modern C++"
section_id: blog
date:   2018-04-23 13:30:00
author: Open GEE Development Team
---

<br />

Hello Everyone!

The Open GEE Development Team would like to make a special announcement about an
effort to upgrade our usage of `C++` to a more modern standard. As of release
`Open GEE 5.2.3`, we will begin compiling Open GEE code using the `C++11` standard.

`C++11` was a major new release of `C++` with many new features to help with
better memory management, performance, and developer productivity. We want the
development experience on `Open GEE` to be a positive and fun experience. Using
standards that are 20 years old isn't fun especially when the newer standards
offer many benefits therefore we felt this change was needed.

To ensure a smoother transition we are asking that for the duration of
`Open GEE 5.2.3` development, code still remains compilable and functioning as
expected using the older `C++98` standard. However, once `Open GEE 5.2.3` branch
is created then any new code pushed into master and any other future branches is
free to use the `C++11` syntax without this restriction and we will no longer
support compiling Open GEE `C++` code using the `C++98` standard.

We wanted to establish this transition period to be sure there were no major
side effects from changing to the new `C++11` compilation flag. In some cases we
had to add `C++11` syntax but we were careful to make sure that the code still
compiled and functioned as expected using `C++98` standards too.

The Open GEE Development Team is committed to keeping the Open GEE source code
relevant and updated. We will continue to make future enhancements to bring the
source code up to more modern standards and technologies.

-Open GEE Development Team
