#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(libsgl, 1.0, reed@google.com)
AM_INIT_AUTOMAKE(libsgl, 1.0)
AC_CONFIG_SRCDIR([makepng.cpp])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lc':
AC_CHECK_LIB([c], [fopen])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC

AC_C_BIGENDIAN(CPPFLAGS="$CPPFLAGS -DSK_CPU_BENDIAN",,)

images=false

AC_ARG_WITH(gif,AC_HELP_STRING([--with-gif],[enable native gif decoding support (FALSE)]),
    [case "${withval}" in
        yes) images=true
             gif=true ;;
        no)  gif=false ;;
        *) AC_MSG_ERROR(bad value ${withval} for --with-gif) ;;
     esac], [gif=false])
if test "$with_gif" == 'yes'; then
    have_gif_header='no'
    AC_CHECK_HEADER(gif_lib.h, have_gif_header='yes', have_gif_header='no')
    if test "$have_gif_header" != 'no'; then
        have_libgif='no'
        AC_CHECK_LIB(gif, DGifOpenFileName, have_libgif='yes', have_libgif='no')
        if test "$have_libgif" != 'no'; then
            LDFLAGS="$LDFLAGS -lgif"
        else
            AC_MSG_ERROR([libgif not found!])
        fi
    else
        AC_MSG_ERROR([gif_lib.h not found!])
    fi
fi
AM_CONDITIONAL(GIF, test x$gif = xtrue)

AC_ARG_WITH(png,AC_HELP_STRING([--with-png],[enable native png decoding support (FALSE)]),
    [case "${withval}" in
        yes) images=true
             png=true ;;
        no)  png=false ;;
        *) AC_MSG_ERROR(bad value ${withval} for --with-png) ;;
     esac], [png=false])
if test "$with_png" == 'yes'; then
    have_png_header='no'
    AC_CHECK_HEADER(png.h, have_png_header='yes', have_png_header='no')
    if test "$have_png_header" != 'no'; then
        have_libpng='no'
        AC_CHECK_LIB(png, png_create_read_struct, have_libpng='yes', have_libpng='no', -lm -lz)
        if test "$have_libgif" != 'no'; then
            LDFLAGS="$LDFLAGS -lpng -lm -lz"
        else
            AC_MSG_ERROR([libpng not found!])
        fi
    else
        AC_MSG_ERROR([png.h not found!])
    fi
fi
AM_CONDITIONAL(PNG, test x$png = xtrue)

AC_ARG_WITH(jpeg,AC_HELP_STRING([--with-jpeg],[enable native jpeg decoding support (FALSE)]),
    [case "${withval}" in
        yes) images=true
             jpeg=true ;;
        no)  jpeg=false ;;
        *) AC_MSG_ERROR(bad value ${withval} for --with-jpeg) ;;
     esac], [jpeg=false])
if test "$with_jpeg" == 'yes'; then
    have_jpeg_header='no'
    AC_CHECK_HEADER(jpeglib.h, have_jpeg_header='yes', have_jpeg_header='no',)
    if test "$have_jpeg_header" != 'no'; then
        have_libjpeg='no'
        AC_CHECK_LIB(jpeg, jpeg_read_header, have_libjpeg='yes', have_libjpeg='no',)
        if test "$have_libjpeg" != 'no'; then
            LDFLAGS="$LDFLAGS -ljpeg"
        else
            AC_MSG_ERROR([libjpeg not found!])
        fi
    else
        AC_MSG_ERROR([jpeglib.h not found!])
    fi
fi
AM_CONDITIONAL(JPEG, test x$jpeg = xtrue)

AM_CONDITIONAL(IMAGES, test x$images = xtrue)

AC_ARG_WITH(freetype,AC_HELP_STRING([--with-freetype],[use freetype for font handling. requires --with-fontpath (TRUE)]),
    [case "${withval}" in
        yes) freetype=true ;;
        no)  freetype=false ;;
        *) AC_MSG_ERROR(bad value ${withval} for --with-freetype) ;;
     esac], [freetype=true])
if test "$freetype" == 'true'; then
    AC_CHECK_LIB([freetype], [FT_Init_FreeType])
    freetype_config=''
    freetype_prefix=''
    AC_CHECK_PROGS(freetype_config, freetype-config,)
    if test -n "$freetype_config"
    then
        freetype_prefix=`freetype-config --prefix`
        freetype_exec_prefix=`freetype-config --exec-prefix`
        LDFLAGS="$LDFLAGS -L${freetype_exec_prefix}/lib"
        CPPFLAGS="$CPPFLAGS -I${freetype_prefix}/include/freetype2"
    fi

    AC_ARG_WITH(fontpath,AC_HELP_STRING([--with-fontpath],[defines SK_FONTPATH -- must be specified if --enable-fontpath=true]),
        CPPFLAGS="$CPPFLAGS -DSK_FONTPATH='\"$with_fontpath\"'",
        AC_MSG_ERROR([must specify --with-fontpath=path/to/font]))

fi
AM_CONDITIONAL(FREETYPE, test x$freetype = xtrue)

AC_ARG_ENABLE(effects,AC_HELP_STRING([--enable-effects],[enable all sgl effects (FALSE)]),
    [case "${enableval}" in
        yes) effects=true ;;
        no)  effects=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-effects) ;;
     esac], [effects=false])
AM_CONDITIONAL(EFFECTS, test x$effects = xtrue)

AC_ARG_ENABLE(float,AC_HELP_STRING([--enable-float],[SkScalar is float (FALSE)]),
    [case "${enableval}" in
        yes) AC_DEFINE([SK_SCALAR_IS_FLOAT]) ;;
        no)  AC_DEFINE([SK_SCALAR_IS_FIXED]) ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-float) ;;
     esac], AC_DEFINE([SK_SCALAR_IS_FIXED]))

AC_CHECK_HEADERS([float.h stdint.h stdlib.h string.h sys/time.h])

AC_ARG_ENABLE(debug,AC_HELP_STRING([--enable-debug],[Enable debugging (FALSE)]),
    [case "${enableval}" in
        yes) AC_DEFINE([DEBUG]) ;;
        no) AC_DEFINE([NDEBUG]) ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
    esac], AC_DEFINE([NDEBUG]))

AC_DEFINE([SK_CAN_USE_FLOAT])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memmove memset sqrt strchr])

AC_CONFIG_FILES(Makefile src/Makefile include/Makefile)
AC_OUTPUT
