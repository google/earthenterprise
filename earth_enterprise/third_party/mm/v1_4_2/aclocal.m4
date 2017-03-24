## ====================================================================
## Copyright (c) 1999-2006 Ralf S. Engelschall <rse@engelschall.com>
## Copyright (c) 1999-2006 The OSSP Project <http://www.ossp.org/>
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
##
## 1. Redistributions of source code must retain the above copyright
##    notice, this list of conditions and the following disclaimer.
##
## 2. Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimer in
##    the documentation and/or other materials provided with the
##    distribution.
##
## 3. All advertising materials mentioning features or use of this
##    software must display the following acknowledgment:
##    "This product includes software developed by
##     Ralf S. Engelschall <rse@engelschall.com>."
##
## 4. Redistributions of any form whatsoever must retain the following
##    acknowledgment:
##    "This product includes software developed by
##     Ralf S. Engelschall <rse@engelschall.com>."
##
## THIS SOFTWARE IS PROVIDED BY RALF S. ENGELSCHALL ``AS IS'' AND ANY
## EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
## PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL RALF S. ENGELSCHALL OR
## ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
## SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
## NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
## HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
## STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
## OF THE POSSIBILITY OF SUCH DAMAGE.
## ====================================================================

define(AC_CHECK_DEBUGGING,[dnl
AC_MSG_CHECKING(for compilation debug mode)
AC_ARG_ENABLE(debug,dnl
[  --enable-debug          build for debugging (default=no)],
[dnl
if test ".$ac_cv_prog_gcc" = ".yes"; then
    case "$CFLAGS" in
        *-O2* ) ;;
            * ) CFLAGS="$CFLAGS -O2" ;;
    esac
    case "$CFLAGS" in
        *-g* ) ;;
           * ) CFLAGS="$CFLAGS -g" ;;
    esac
    CFLAGS="$CFLAGS -Wall -Wshadow -Wpointer-arith -Wcast-align"
    CFLAGS="$CFLAGS -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline"
else
    case "$CFLAGS" in
        *-g* ) ;;
           * ) CFLAGS="$CFLAGS -g" ;;
    esac
fi
msg="enabled"
AC_DEFINE(MM_DEBUG, 1, [define to enable debugging])
],[
case "$CFLAGS" in
    *-g* ) CFLAGS=`echo "$CFLAGS" |\
                   sed -e 's/ -g / /g' -e 's/ -g$//' -e 's/^-g //g' -e 's/^-g$//'` ;;
esac
msg=disabled
])dnl
AC_MSG_RESULT([$msg])
])

define(AC_CONFIGURE_PART,[dnl
AC_MSG_RESULT()
AC_MSG_RESULT(${T_MD}$1:${T_ME})
])dnl

define(AC_CHECK_DEFINE,[dnl
  AC_CACHE_CHECK(for $1 in $2, ac_cv_define_$1,
    AC_EGREP_CPP([YES_IS_DEFINED], [
#include <$2>
#ifdef $1
YES_IS_DEFINED
#endif
    ], ac_cv_define_$1=yes, ac_cv_define_$1=no)
  )
  if test "$ac_cv_define_$1" = "yes" ; then
    AC_DEFINE(HAVE_$1, 1, [define to 1 if you have the $1 define])
  fi
])dnl
AC_DEFINE(HAVE_$1)

define(AC_IFALLYES,[dnl
ac_rc=yes
for ac_spec in $1; do
    ac_type=`echo "$ac_spec" | sed -e 's/:.*$//'`
    ac_item=`echo "$ac_spec" | sed -e 's/^.*://'`
    case $ac_type in
        header )
            ac_item=`echo "$ac_item" | sed 'y%./+-%__p_%'`
            ac_var="ac_cv_header_$ac_item"
            ;;
        file )
            ac_item=`echo "$ac_item" | sed 'y%./+-%__p_%'`
            ac_var="ac_cv_file_$ac_item"
            ;;
        func )   ac_var="ac_cv_func_$ac_item"   ;;
        define ) ac_var="ac_cv_define_$ac_item" ;;
        custom ) ac_var="$ac_item" ;;
    esac
    eval "ac_val=\$$ac_var"
    if test ".$ac_val" != .yes; then
        ac_rc=no
        break
    fi
done
if test ".$ac_rc" = .yes; then
    :
    $2
else
    :
    $3
fi
])dnl

define(AC_BEGIN_DECISION,[dnl
ac_decision_item='$1'
ac_decision_msg='FAILED'
ac_decision=''
])dnl
define(AC_DECIDE,[dnl
ac_decision='$1'
ac_decision_msg='$2'
ac_decision_$1=yes
ac_decision_$1_msg='$2'
])dnl
define(AC_DECISION_OVERRIDE,[dnl
    ac_decision=''
    for ac_item in $1; do
         eval "ac_decision_this=\$ac_decision_${ac_item}"
         if test ".$ac_decision_this" = .yes; then
             ac_decision=$ac_item
             eval "ac_decision_msg=\$ac_decision_${ac_item}_msg"
         fi
    done
])dnl
define(AC_DECISION_FORCE,[dnl
ac_decision="$1"
eval "ac_decision_msg=\"\$ac_decision_${ac_decision}_msg\""
])dnl
define(AC_END_DECISION,[dnl
if test ".$ac_decision" = .; then
    echo "[$]0:Error: decision on $ac_decision_item failed" 1>&2
    exit 1
else
    if test ".$ac_decision_msg" = .; then
        ac_decision_msg="$ac_decision"
    fi
    AC_MSG_RESULT([decision on $ac_decision_item... $ac_decision_msg])
fi
])dnl

AC_DEFUN(AC_TEST_FILE,
[AC_REQUIRE([AC_PROG_CC])
ac_safe=`echo "$1" | sed 'y%./+-%__p_%'`
AC_MSG_CHECKING([for $1])
AC_CACHE_VAL(ac_cv_file_$ac_safe, [
  if test -r $1; then
    eval "ac_cv_file_$ac_safe=yes"
  else
    eval "ac_cv_file_$ac_safe=no"
  fi
])dnl
if eval "test \"`echo '$ac_cv_file_'$ac_safe`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
ifelse([$3], , , [$3])
fi
])

AC_DEFUN(AC_PROG_NM,
[AC_MSG_CHECKING([for BSD-compatible nm])
AC_CACHE_VAL(ac_cv_path_NM,
[if test -n "$NM"; then
  # Let the user override the test.
  ac_cv_path_NM="$NM"
else
  IFS="${IFS=   }"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  for ac_dir in /usr/ucb /usr/ccs/bin $PATH /bin; do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/nm; then
      # Check to see if the nm accepts a BSD-compat flag.
      # Adding the `sed 1q' prevents false positives on HP-UX, which says:
      #   nm: unknown option "B" ignored
      if ($ac_dir/nm -B /dev/null 2>&1 | sed '1q'; exit 0) | egrep /dev/null >/dev/null; then
        ac_cv_path_NM="$ac_dir/nm -B"
      elif ($ac_dir/nm -p /dev/null 2>&1 | sed '1q'; exit 0) | egrep /dev/null >/dev/null; then
        ac_cv_path_NM="$ac_dir/nm -p"
      else
        ac_cv_path_NM="$ac_dir/nm"
      fi
      break
    fi
  done
  IFS="$ac_save_ifs"
  test -z "$ac_cv_path_NM" && ac_cv_path_NM=nm
fi])
NM="$ac_cv_path_NM"
AC_MSG_RESULT([$NM])
AC_SUBST(NM)
])

define(AC_CHECK_MAXSEGSIZE,[dnl
AC_MSG_CHECKING(for shared memory maximum segment size)
AC_CACHE_VAL(ac_cv_maxsegsize,[
OCFLAGS="$CFLAGS"
case "$1" in
    MM_SHMT_MM*    ) CFLAGS="-DTEST_MMAP   $CFLAGS" ;;
    MM_SHMT_IPCSHM ) CFLAGS="-DTEST_SHMGET $CFLAGS" ;;
esac
cross_compile=no
AC_TRY_RUN(
changequote(<<, >>)dnl
<<
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef TEST_MMAP
#include <sys/mman.h>
#endif
#ifdef TEST_SHMGET
#ifdef MM_OS_SUNOS
#define KERNEL 1
#endif
#ifdef MM_OS_BS2000
#define _KMEMUSER
#endif
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef MM_OS_SUNOS
#undef KERNEL
#endif
#ifdef MM_OS_BS2000
#undef _KMEMUSER
#endif
#if !defined(SHM_R)
#define SHM_R 0400
#endif
#if !defined(SHM_W)
#define SHM_W 0200
#endif
#endif
#if !defined(MAP_FAILED)
#define MAP_FAILED ((void *)(-1))
#endif
#ifdef MM_OS_BEOS
#include <kernel/OS.h>
#endif

int testit(int size)
{
    int fd;
    void *segment;
#ifdef TEST_MMAP
    char file[] = "./ac_test.tmp";
    unlink(file);
    if ((fd = open(file, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR)) == -1)
        return 0;
    if (ftruncate(fd, size) == -1)
        return 0;
    if ((segment = (void *)mmap(NULL, size, PROT_READ|PROT_WRITE,
                                MAP_SHARED, fd, 0)) == (void *)MAP_FAILED) {
        close(fd);
        return 0;
    }
    munmap((caddr_t)segment, size);
    close(fd);
    unlink(file);
#endif
#ifdef TEST_SHMGET
    if ((fd = shmget(IPC_PRIVATE, size, SHM_R|SHM_W|IPC_CREAT)) == -1)
        return 0;
    if ((segment = (void *)shmat(fd, NULL, 0)) == ((void *)-1)) {
        shmctl(fd, IPC_RMID, NULL);
        return 0;
    }
    shmdt(segment);
    shmctl(fd, IPC_RMID, NULL);
#endif
#ifdef TEST_BEOS
    area_id id;
    id = create_area("mm_test", (void*)&segment, B_ANY_ADDRESS, size,
                     B_LAZY_LOCK, B_READ_AREA|B_WRITE_AREA);
    if (id < 0)
        return 0;
    delete_area(id);
#endif
    return 1;
}

#define ABS(n) ((n) >= 0 ? (n) : (-(n)))

int main(int argc, char *argv[])
{
    int t, m, b;
    int d;
    int rc;
    FILE *f;

    /*
     * Find maximum possible allocation size by performing a
     * binary search starting with a search space between 0 and
     * 64MB of memory.
     */
    t = 1024*1024*64 /* = 67108864 */;
    if (testit(t))
        m = t;
    else {
        m = 1024*1024*32;
        b = 0;
        for (;;) {
            /* fprintf(stderr, "t=%d, m=%d, b=%d\n", t, m, b); */
            rc = testit(m);
            if (rc) {
                d = ((t-m)/2);
                b = m;
            }
            else {
                d = -((m-b)/2);
                t = m;
            }
            if (ABS(d) < 1024*1) {
                if (!rc)
                    m = b;
                break;
            }
            if (m < 1024*8)
                break;
            m += d;
        }
        if (m < 1024*8)
            m = 0;
    }
    if ((f = fopen("conftestval", "w")) == NULL)
        exit(1);
    fprintf(f, "%d\n", m);
    fclose(f);
    exit(0);
}
>>
changequote([, ])dnl
,[ac_cv_maxsegsize="`cat conftestval`"
],
ac_cv_maxsegsize=0
,
ac_cv_maxsegsize=0
)
CFLAGS="$OCFLAGS"
])
msg="$ac_cv_maxsegsize"
if test $msg -eq 67108864; then
    msg="64MB (soft limit)"
elif test $msg -gt 1048576; then
    msg="`expr $msg / 1024`"
    msg="`expr $msg / 1024`"
    msg="${msg}MB"
elif test $msg -gt 1024; then
    msg="`expr $msg / 1024`"
    msg="${msg}KB"
else
    ac_cv_maxsegsize=0
    msg=unknown
fi
MM_SHM_MAXSEGSIZE=$ac_cv_maxsegsize
test ".$msg" = .unknown && AC_MSG_ERROR([Unable to determine maximum shared memory segment size])
AC_MSG_RESULT([$msg])
AC_DEFINE_UNQUOTED(MM_SHM_MAXSEGSIZE, $MM_SHM_MAXSEGSIZE, [maximum segment size])
])

