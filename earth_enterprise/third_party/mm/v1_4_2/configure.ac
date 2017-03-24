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

dnl ##
dnl ##   Autoconf specification for OSSP mm library
dnl ##

dnl #
dnl #   standard Autoconf prolog
dnl #

dnl #   autoconf initialization
AC_PREREQ(2.53)dnl
AC_INIT(README)
AC_PREFIX_DEFAULT(/usr/local)

dnl #   shtool bootstrap
SHTOOL="\$(TOP)/shtool"
AC_SUBST(SHTOOL)
ac_shtool="./shtool"
T_MD=`$ac_shtool echo -n -e %B`
T_ME=`$ac_shtool echo -n -e %b`
PLATFORM=`${CONFIG_SHELL-/bin/sh} ./config.guess`
PLATFORM=`${CONFIG_SHELL-/bin/sh} ./config.sub $PLATFORM`
MM_VERSION_STR="`$ac_shtool version -l c -d long mm_vers.c`"
AC_SUBST(MM_VERSION_STR)

dnl #   friendly header ;-)
echo "Configuring ${T_MD}OSSP mm${T_ME} (Shared Memory Allocation), Version ${T_MD}${MM_VERSION_STR}${T_ME}"
echo "Copyright (c) 1999-2006 Ralf S. Engelschall <rse@engelschall.com>"
echo "Copyright (c) 1999-2006 The OSSP Project <http://www.ossp.org/>"
echo "Platform: ${T_MD}${PLATFORM}${T_ME}"

dnl #
dnl #   determine build tools and parameters
dnl #

AC_CONFIGURE_PART(Build Tools)
AC_PROG_CC
AC_PROG_CPP
AC_CHECK_DEBUGGING
AC_SET_MAKE

sinclude(libtool.m4)
AC_PROG_LIBTOOL

dnl #   support for some special platform/compiler options
case "$PLATFORM:$CC" in
    *-hp-hpux*:cc ) CFLAGS="$CFLAGS -Ae +DAportable" ;;
esac

dnl #
dnl #   determine system parameters
dnl #

AC_CONFIGURE_PART(Platform Environment)

AC_HAVE_HEADERS(stdio.h stdlib.h string.h dnl
                errno.h limits.h unistd.h fcntl.h dnl
                sys/stat.h sys/types.h)
AC_BEGIN_DECISION([mandatory system headers])
AC_IFALLYES(header:stdio.h header:stdlib.h header:string.h dnl
            header:errno.h header:limits.h header:unistd.h header:fcntl.h dnl
            header:sys/stat.h header:sys/types.h,
            AC_DECIDE(fine, [all fine]))
AC_END_DECISION

AC_HAVE_HEADERS(memory.h)
AC_CHECK_FUNCS(memcpy memset bcopy)
AC_CHECK_DEFINE(_POSIX_PATH_MAX, limits.h)
AC_CHECK_DEFINE(PATH_MAX, limits.h)
AC_CHECK_DEFINE(_POSIX_CHILD_MAX, limits.h)
AC_CHECK_DEFINE(CHILD_MAX, limits.h)
AC_HAVE_HEADERS(sys/param.h)
AC_CHECK_DEFINE(MAXPATHLEN, sys/param.h)

dnl #   some special defines for brain dead platforms
case $PLATFORM in
   *-*-sunos* ) AC_DEFINE(MM_OS_SUNOS,  1, [define to 1 for SunOS 4.x support]) ;;
   BS2000-*-* ) AC_DEFINE(MM_OS_BS2000, 1, [define to 1 for BS2000 support])    ;;
   *-*-beos*  ) AC_DEFINE(MM_OS_BEOS,   1, [define to 1 for BeOS support])      ;;
esac

dnl #
dnl #   method to determine virtual memory page size
dnl #

AC_CONFIGURE_PART(Virtual Memory Page Size)

AC_HAVE_HEADERS(unistd.h kernel/OS.h)
AC_HAVE_FUNCS(getpagesize sysconf)
AC_CHECK_DEFINE(_SC_PAGESIZE, unistd.h)
AC_CHECK_DEFINE(B_PAGE_SIZE, kernel/OS.h)

AH_TEMPLATE([MM_VMPS_GETPAGESIZE], [memory page size determination: 4.2BSD getpagesize()])
AH_TEMPLATE([MM_VMPS_SYSCONF],     [memory page size determination: POSIX.1 sysconf(_SC_PAGESIZE)])
AH_TEMPLATE([MM_VMPS_BEOS],        [memory page size determination: BeOS B_PAGE_SIZE])
AC_BEGIN_DECISION([memory page size determination])
AC_IFALLYES(header:unistd.h func:getpagesize,
            AC_DECIDE(MM_VMPS_GETPAGESIZE, [4.2BSD getpagesize()]))
AC_IFALLYES(header:unistd.h func:sysconf define:_SC_PAGESIZE,
            AC_DECIDE(MM_VMPS_SYSCONF, [POSIX.1 sysconf(_SC_PAGESIZE)]))
AC_IFALLYES(header:kernel/OS.h define:B_PAGE_SIZE,
            AC_DECIDE(MM_VMPS_BEOS, [BeOS B_PAGE_SIZE]))
AC_END_DECISION
AC_DEFINE_UNQUOTED($ac_decision)

dnl #
dnl #   Shared Memory
dnl #

AC_CONFIGURE_PART(Shared Memory Implementation)

AC_HAVE_HEADERS(sys/mman.h)
AC_CHECK_DEFINE(MAP_ANON, sys/mman.h)
AC_HAVE_FUNCS(mmap munmap shm_open shm_unlink)
AC_TEST_FILE(/dev/zero)
AC_HAVE_HEADERS(sys/ipc.h sys/shm.h sys/file.h)
AC_HAVE_FUNCS(shmget shmat shmdt shmctl)
AC_HAVE_HEADERS(kernel/OS.h)
AC_HAVE_FUNCS(create_area)

AH_TEMPLATE([MM_SHMT_BEOS],   [shared memory allocation method: BeOS areas])
AH_TEMPLATE([MM_SHMT_MMFILE], [shared memory allocation method: Classical mmap() on temporary file])
AH_TEMPLATE([MM_SHMT_MMPOSX], [shared memory allocation method: mmap() via POSIX.1 shm_open() on temporary file])
AH_TEMPLATE([MM_SHMT_MMZERO], [shared memory allocation method: SVR4-style mmap() on /dev/zero])
AH_TEMPLATE([MM_SHMT_IPCSHM], [shared memory allocation method: SysV IPC shmget()])
AH_TEMPLATE([MM_SHMT_MMANON], [shared memory allocation method: 4.4BSD-style mmap() via MAP_ANON])
AC_BEGIN_DECISION([shared memory allocation method])
AC_IFALLYES(header:kernel/OS.h func:create_area,
            AC_DECIDE(MM_SHMT_BEOS, [BeOS areas]))
AC_IFALLYES(header:sys/mman.h func:mmap func:munmap,
            AC_DECIDE(MM_SHMT_MMFILE, [Classical mmap() on temporary file]))
AC_IFALLYES(header:sys/mman.h func:mmap func:munmap func:shm_open func:shm_unlink,
            AC_DECIDE(MM_SHMT_MMPOSX, [mmap() via POSIX.1 shm_open() on temporary file]))
AC_IFALLYES(header:sys/mman.h func:mmap func:munmap file:/dev/zero,
            AC_DECIDE(MM_SHMT_MMZERO, [SVR4-style mmap() on /dev/zero]))
AC_IFALLYES(header:sys/ipc.h header:sys/shm.h header:sys/file.h dnl
            func:shmget func:shmat func:shmdt func:shmctl,
            AC_DECIDE(MM_SHMT_IPCSHM, [SysV IPC shmget()]))
AC_IFALLYES(header:sys/mman.h func:mmap func:munmap define:MAP_ANON,
            AC_DECIDE(MM_SHMT_MMANON, [4.4BSD-style mmap() via MAP_ANON]))
case $PLATFORM in
    *-*-linux* )
        #   Unfortunately, Linux has many problems, so we have to do
        #   more granular manual decisions here. The background is:
        #   MM_SHMT_IPCSHM:
        #       Supported under Linux 2.0 and upwards. Works out of the
        #       box on Linux 2.2, but requires an explicitly mounted shm
        #       filesystem under 2.3 only. The requirement for the shm
        #       filesystem should be gone with Linux 2.4.
        #   MM_SHMT_MMANON:
        #       Not allowed up to and included to Linux 2.2.
        #       Should be supported in Linux 2.3, but still hangs there.
        #       Will be supported from Linux 2.4 on the first time really.
        #   MM_SHMT_MMZERO:
        #       Not allowed up to and included to Linux 2.2.
        #       Should be supported in Linux 2.3, but still hangs there.
        #       Will be supported from Linux 2.4 on the first time really.
        #   MM_SHMT_MMFILE:
        #       Works on Linux versions.
        case $PLATFORM in
            *-linux1* )
                AC_DECISION_FORCE(MM_SHMT_MMFILE)
                ;;
            *-*-linux2.0* )
                AC_DECISION_FORCE(MM_SHMT_IPCSHM)
                ;;
            *-*-linux2.[12]* )
                AC_DECISION_FORCE(MM_SHMT_IPCSHM)
                ;;
            *-*-linux2.3* )
                if test ".`df 2>/dev/null | grep shm`" != .; then
                    AC_DECISION_FORCE(MM_SHMT_IPCSHM)
                else
                    AC_DECISION_FORCE(MM_SHMT_MMFILE)
                fi
                ;;
            * )
                AC_DECISION_OVERRIDE(MM_SHMT_MMFILE MM_SHMT_MMZERO MM_SHMT_MMPOSX MM_SHMT_MMANON MM_SHMT_IPCSHM)
        esac
        ;;
esac
AC_ARG_WITH(shm,dnl
[  --with-shm=TYPE         force shared memory type: MMFILE MMZERO MMPOSX MMANON IPCSHM BEOS],
AC_DECISION_FORCE(MM_SHMT_$withval)
)dnl
AC_END_DECISION
AC_DEFINE_UNQUOTED($ac_decision)

AC_CHECK_MAXSEGSIZE($ac_decision)

dnl #
dnl #   Mutex
dnl #

AC_CONFIGURE_PART(Mutual Exclusion Implementation)

AC_HAVE_HEADERS(sys/ipc.h sys/sem.h sys/file.h)
AC_HAVE_FUNCS(semget semctl)
AC_CHECK_DEFINE(LOCK_EX, sys/file.h)
AC_CHECK_DEFINE(F_SETLK, fcntl.h)
AC_CHECK_DEFINE(IPC_PRIVATE, sys/ipc.h)
AC_CHECK_DEFINE(SEM_UNDO, sys/sem.h)
AC_HAVE_HEADERS(kernel/OS.h)
AC_CHECK_FUNCS(create_sem)

AC_MSG_CHECKING(whether union semun is defined in sys/sem.h)
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
],[
union semun arg;
semctl(0, 0, 0, arg);
],
AC_DEFINE(HAVE_UNION_SEMUN, 1, [define to 1 if you have union semun])
msg=yes,dnl
msg=no)
AC_MSG_RESULT([$msg])

AH_TEMPLATE([MM_SEMT_FLOCK],  [mutex implementation method: 4.2BSD-style flock() on temporary file])
AH_TEMPLATE([MM_SEMT_IPCSEM], [mutex implementation method: SysV IPC semget()])
AH_TEMPLATE([MM_SEMT_FCNTL],  [mutex implementation method: SVR4-style fcntl() on temporary file])
AH_TEMPLATE([MM_SEMT_BEOS],   [mutex implementation method: BeOS semaphores])
AC_BEGIN_DECISION([mutex implementation method])
AC_IFALLYES(header:sys/file.h define:LOCK_EX,
            AC_DECIDE(MM_SEMT_FLOCK, [4.2BSD-style flock() on temporary file]))
AC_IFALLYES(header:sys/ipc.h header:sys/sem.h header:sys/file.h dnl
            func:semget func:semctl,
            AC_DECIDE(MM_SEMT_IPCSEM, [SysV IPC semget()]))
AC_IFALLYES(header:fcntl.h define:F_SETLK,
            AC_DECIDE(MM_SEMT_FCNTL, [SVR4-style fcntl() on temporary file]))
AC_IFALLYES(header:kernel/OS.h func:create_sem,
            AC_DECIDE(MM_SEMT_BEOS, [BeOS semaphores]))
AC_ARG_WITH(sem,dnl
[  --with-sem=TYPE         force semaphore type: FLOCK FCNTL IPCSEM BEOS],
AC_DECISION_FORCE(MM_SEMT_$withval)
)dnl
AC_END_DECISION
AC_DEFINE_UNQUOTED($ac_decision)

dnl #
dnl #   finally: source file substitution...
dnl #

AC_CONFIGURE_PART(Output Substitution)

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([Makefile mm-config])
AC_CONFIG_COMMANDS([adjustment], [chmod a+x mm-config])
AC_OUTPUT

