/* ====================================================================
 * Copyright (c) 1999-2006 Ralf S. Engelschall <rse@engelschall.com>
 * Copyright (c) 1999-2006 The OSSP Project <http://www.ossp.org/>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by
 *     Ralf S. Engelschall <rse@engelschall.com>."
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by
 *     Ralf S. Engelschall <rse@engelschall.com>."
 *
 * THIS SOFTWARE IS PROVIDED BY RALF S. ENGELSCHALL ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL RALF S. ENGELSCHALL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 */

/*
**
**  mm_core.c -- Low-level Shared Memory API
**
*/

#define MM_PRIVATE
#include "mm.h"

/*
 * Some global variables
 */

#if defined(MM_SEMT_FCNTL)
/* lock/unlock structures for fcntl() */
static struct flock mm_core_dolock_rd;
static struct flock mm_core_dolock_rw;
static struct flock mm_core_dounlock;
#endif

#if defined(MM_SEMT_IPCSEM)
/* lock/unlock structures for semop() */
static union  semun  mm_core_semctlarg;
static struct sembuf mm_core_dolock[2];
static struct sembuf mm_core_dounlock[1];
#endif

#if defined(MM_SHMT_MMFILE) || defined(MM_SHMT_MMPOSX)
static size_t mm_core_mapoffset = 0;           /* we use own file */
#elif defined(MM_SHMT_MMZERO)
static size_t mm_core_mapoffset = 1024*1024*1; /* we share with other apps */
#endif

static void mm_core_init(void)
{
    static int initialized = FALSE;
    if (!initialized) {
#if defined(MM_SEMT_FCNTL)
        mm_core_dolock_rd.l_whence   = SEEK_SET; /* from current point */
        mm_core_dolock_rd.l_start    = 0;        /* -"- */
        mm_core_dolock_rd.l_len      = 0;        /* until end of file */
        mm_core_dolock_rd.l_type     = F_RDLCK;  /* set shard/read lock */
        mm_core_dolock_rd.l_pid      = 0;        /* pid not actually interesting */
        mm_core_dolock_rw.l_whence   = SEEK_SET; /* from current point */
        mm_core_dolock_rw.l_start    = 0;        /* -"- */
        mm_core_dolock_rw.l_len      = 0;        /* until end of file */
        mm_core_dolock_rw.l_type     = F_WRLCK;  /* set exclusive/read-write lock */
        mm_core_dolock_rw.l_pid      = 0;        /* pid not actually interesting */
        mm_core_dounlock.l_whence    = SEEK_SET; /* from current point */
        mm_core_dounlock.l_start     = 0;        /* -"- */
        mm_core_dounlock.l_len       = 0;        /* until end of file */
        mm_core_dounlock.l_type      = F_UNLCK;  /* unlock */
        mm_core_dounlock.l_pid       = 0;        /* pid not actually interesting */
#endif
#if defined(MM_SEMT_IPCSEM)
        mm_core_dolock[0].sem_num   = 0;
        mm_core_dolock[0].sem_op    = 0;
        mm_core_dolock[0].sem_flg   = 0;
        mm_core_dolock[1].sem_num   = 0;
        mm_core_dolock[1].sem_op    = 1;
        mm_core_dolock[1].sem_flg   = SEM_UNDO;
        mm_core_dounlock[0].sem_num = 0;
        mm_core_dounlock[0].sem_op  = -1;
        mm_core_dounlock[0].sem_flg = SEM_UNDO;
#endif
        initialized = TRUE;
    }
    return;
}

#if defined(MM_SEMT_FLOCK)
/*
 * Determine per-process fd for semaphore
 * (needed only for flock() based semaphore)
 */
static int mm_core_getfdsem(mem_core *mc)
{
    int fd = -1;
    pid_t pid;
    int i;

    pid = getpid();
    for (i = 0; i < MM_MAXCHILD &&
                mc->mc_fdsem[i].pid != 0 &&
                mc->mc_fdsem[i].fd != -1; i++) {
        if (mc->mc_fdsem[i].pid == pid) {
            fd = mc->mc_fdsem[i].fd;
            break;
        }
    }
    if (fd == -1 && i < MM_MAXCHILD) {
        fd = open(mc->mc_fnsem, O_WRONLY, MM_CORE_FILEMODE);
#if defined(F_SETFD) && defined(FD_CLOEXEC)
        fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
        mc->mc_fdsem[i].pid = getpid();
        mc->mc_fdsem[i].fd  = fd;
    }
    return fd;
}
#endif /* MM_SEMT_FLOCK */

/*
 * Determine memory page size of OS
 */

static size_t mm_core_pagesize(void)
{
    static int pagesize = 0;
    if (pagesize == 0)
#if defined(MM_VMPS_GETPAGESIZE)
        pagesize = getpagesize();
#elif defined(MM_VMPS_SYSCONF)
        pagesize = sysconf(_SC_PAGESIZE);
#elif defined(MM_VMPS_BEOS)
        pagesize = B_PAGE_SIZE;
#else
        pagesize = MM_CORE_DEFAULT_PAGESIZE;
#endif
    return pagesize;
}

/*
 * Align a size to the next page or word boundary
 */

size_t mm_core_align2page(size_t size)
{
    int psize = mm_core_pagesize();
    return ((size)%(psize) > 0 ? ((size)/(psize)+1)*(psize) : (size));
}

size_t mm_core_align2word(size_t size)
{
    return ((1+((size-1) / SIZEOF_mem_word)) * SIZEOF_mem_word);
}

size_t mm_core_maxsegsize(void)
{
    return mm_core_align2page((MM_SHM_MAXSEGSIZE-SIZEOF_mem_core)-mm_core_pagesize());
}

/*
 * Create a shared memory area
 */

void *mm_core_create(size_t usersize, const char *file)
{
    mem_core *mc;
    void *area = ((void *)-1);
#if defined(MM_SHMT_MMPOSX) || defined(MM_SHMT_MMZERO) || defined(MM_SHMT_MMFILE) || defined(MM_SHMT_IPCSHM)
    int fdmem = -1;
#endif
    int fdsem = -1;
#if defined(MM_SEMT_IPCSEM)
    int fdsem_rd = -1;
#endif
#if defined(MM_SHMT_MMPOSX) || defined(MM_SHMT_MMFILE)
    char *fnmem;
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    char *fnsem;
#endif
    size_t size;
#if defined(MM_SHMT_MMZERO) || defined(MM_SHMT_MMPOSX) || defined(MM_SHMT_MMFILE)
    int zero = 0;
#endif
#if defined(MM_SHMT_IPCSHM)
    struct shmid_ds shmbuf;
#endif
#if defined(MM_SHMT_MMPOSX) || defined(MM_SHMT_MMFILE)
    char shmfilename[MM_MAXPATH];
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    char semfilename[MM_MAXPATH];
#endif
#if defined(MM_SHMT_BEOS)
    area_id temparea;
#endif
    char filename[MM_MAXPATH];

    if (usersize <= 0 || usersize > mm_core_maxsegsize()) {
        errno = EINVAL;
        return NULL;
    }
    if (file == NULL) {
        sprintf(filename, MM_CORE_DEFAULT_FILE, (int)getpid());
        file = filename;
    }

    mm_core_init();
    size = mm_core_align2page(usersize+SIZEOF_mem_core);

#if defined(MM_SHMT_MMPOSX) || defined(MM_SHMT_MMFILE)
    sprintf(shmfilename, "%s.mem", file);
    fnmem = shmfilename;
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    sprintf(semfilename, "%s.sem", file);
    fnsem = semfilename;
#endif

#if defined(MM_SHMT_MMANON)
    if ((area = (void *)mmap(NULL, size, PROT_READ|PROT_WRITE,
                             MAP_ANON|MAP_SHARED, -1, 0)) == (void *)MAP_FAILED)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to memory map anonymous area");
#endif /* MM_SHMT_MMANON */

#if defined(MM_SHMT_BEOS)
    if ((temparea = create_area("mm", (void*)&area, B_ANY_ADDRESS,
                                size, B_LAZY_LOCK, B_READ_AREA|B_WRITE_AREA)) < 0)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to create the memory area");
#endif /* MM_SHMT_BEOS */

#if defined(MM_SHMT_MMPOSX)
    shm_unlink(fnmem); /* Ok when it fails */
    if ((fdmem = shm_open(fnmem, O_RDWR|O_CREAT|O_EXCL, MM_CORE_FILEMODE)) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to open tempfile");
    if (ftruncate(fdmem, mm_core_mapoffset+size) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to truncate tempfile");
    write(fdmem, &zero, sizeof(zero));
    if ((area = (void *)mmap(NULL, size, PROT_READ|PROT_WRITE,
                             MAP_SHARED, fdmem, mm_core_mapoffset)) == (void *)MAP_FAILED)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to memory map tempfile");
    shm_unlink(fnmem);
    close(fdmem); fdmem = -1;
    mm_core_mapoffset += size;
#endif /* MM_SHMT_MMPOSX */

#if defined(MM_SHMT_MMZERO)
    if ((fdmem = open("/dev/zero", O_RDWR, MM_CORE_FILEMODE)) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to open /dev/zero");
    if (lseek(fdmem, mm_core_mapoffset+size, SEEK_SET) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to seek in /dev/zero");
    write(fdmem, &zero, sizeof(zero));
    if ((area = (void *)mmap(NULL, size, PROT_READ|PROT_WRITE,
                             MAP_SHARED, fdmem, mm_core_mapoffset)) == (void *)MAP_FAILED)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to memory map /dev/zero");
    close(fdmem); fdmem = -1;
    mm_core_mapoffset += size;
#endif /* MM_SHMT_MMZERO */

#if defined(MM_SHMT_MMFILE)
    unlink(fnmem);
    if ((fdmem = open(fnmem, O_RDWR|O_CREAT|O_EXCL, MM_CORE_FILEMODE)) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to open memory file");
    if (ftruncate(fdmem, mm_core_mapoffset+size) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to truncate memory file");
    write(fdmem, &zero, sizeof(zero));
    if ((area = (void *)mmap(NULL, size, PROT_READ|PROT_WRITE,
                             MAP_SHARED, fdmem, mm_core_mapoffset)) == (void *)MAP_FAILED)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to memory map memory file");
    close(fdmem); fdmem = -1;
    mm_core_mapoffset += size;
#endif /* MM_SHMT_MMFILE */

#if defined(MM_SHMT_IPCSHM)
    if ((fdmem = shmget(IPC_PRIVATE, size, (SHM_R|SHM_W|IPC_CREAT))) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to acquire shared memory segment");
    if ((area = (void *)shmat(fdmem, NULL, 0)) == ((void *)-1))
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to attach shared memory");
    if (shmctl(fdmem, IPC_STAT, &shmbuf) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to get status of shared memory");
    shmbuf.shm_perm.uid = getuid();
    shmbuf.shm_perm.gid = getgid();
    if (shmctl(fdmem, IPC_SET, &shmbuf) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to set status of shared memory");
    if (shmctl(fdmem, IPC_RMID, NULL) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to remove shared memory in advance");
#endif /* MM_SHMT_IPCSHM */

#if defined(MM_SEMT_FLOCK)
    unlink(fnsem);
    if ((fdsem = open(fnsem, O_RDWR|O_CREAT|O_EXCL, MM_CORE_FILEMODE)) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to open semaphore file");
#if defined(F_SETFD) && defined(FD_CLOEXEC)
    if (fcntl(fdsem, F_SETFD, FD_CLOEXEC) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to set close-on-exec flag");
#endif
#endif /* MM_SEMT_FLOCK */

#if defined(MM_SEMT_FCNTL)
    unlink(fnsem);
    if ((fdsem = open(fnsem, O_RDWR|O_CREAT|O_EXCL, MM_CORE_FILEMODE)) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to open semaphore file");
#if defined(F_SETFD) && defined(FD_CLOEXEC)
    if (fcntl(fdsem, F_SETFD, FD_CLOEXEC) == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to set close-on-exec flag");
#endif
#endif /* MM_SEMT_FCNTL */

#if defined(MM_SEMT_IPCSEM)
    fdsem = semget(IPC_PRIVATE, 1, IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR);
    if (fdsem == -1 && errno == EEXIST)
        fdsem = semget(IPC_PRIVATE, 1, IPC_EXCL|S_IRUSR|S_IWUSR);
    if (fdsem == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to acquire semaphore");
    mm_core_semctlarg.val = 0;
    semctl(fdsem, 0, SETVAL, mm_core_semctlarg);
    fdsem_rd = semget(IPC_PRIVATE, 1, IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR);
    if (fdsem_rd == -1 && errno == EEXIST)
        fdsem_rd = semget(IPC_PRIVATE, 1, IPC_EXCL|S_IRUSR|S_IWUSR);
    if (fdsem_rd == -1)
        FAIL(MM_ERR_CORE|MM_ERR_SYSTEM, "failed to acquire semaphore");
    mm_core_semctlarg.val = 0;
    semctl(fdsem_rd, 0, SETVAL, mm_core_semctlarg);
#endif /* MM_SEMT_IPCSEM */

    /*
     * Configure the memory core parameters
     */
    mc = (mem_core *)area;
    mc->mc_size     = size;
    mc->mc_usize    = usersize;
    mc->mc_pid      = getpid();
#if defined(MM_SHMT_IPCSHM)
    mc->mc_fdmem    = fdmem;
#endif
#if defined(MM_SEMT_FLOCK)
    mc->mc_fdsem[0].pid = getpid();
    mc->mc_fdsem[0].fd  = fdsem;
    mc->mc_fdsem[1].pid = 0;
    mc->mc_fdsem[1].fd  = -1;
#else
    mc->mc_fdsem = fdsem;
#endif
#if defined(MM_SEMT_BEOS)
    mc->mc_semid = create_sem(0, "mm_semid");
    mc->mc_ben = 0;
#endif
#if defined(MM_SHMT_BEOS)
    mc->mc_areaid = temparea;
#endif
#if defined(MM_SEMT_IPCSEM)
    mc->mc_fdsem_rd = fdsem_rd;
    mc->mc_readers = 0;
#endif
#if defined(MM_SHMT_MMFILE)
    memcpy(mc->mc_fnmem, fnmem, MM_MAXPATH);
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    memcpy(mc->mc_fnsem, fnsem, MM_MAXPATH);
#endif

    /*
     * Return successfully established core
     */
    return ((void *)&(mc->mc_base.mw_cp));

    /*
     * clean-up sequence (CUS) for error situation
     */
    BEGIN_FAILURE
#if defined(MM_SHMT_MMANON) || defined(MM_SHMT_MMZERO) || defined(MM_SHMT_MMPOSX) || defined(MM_SHMT_MMFILE)
    if (area != ((void *)-1))
        munmap((caddr_t)area, size);
#endif
#if defined(MM_SHMT_IPCSHM)
    if (area != ((void *)-1))
        shmdt(area);
#endif
#if defined(MM_SHMT_MMPOSX) || defined(MM_SHMT_MMZERO) || defined(MM_SHMT_MMFILE)
    if (fdmem != -1)
        close(fdmem);
#endif
#if defined(MM_SEMT_BEOS)
    delete_sem(mc->mc_semid);
#endif
#if defined(MM_SHMT_BEOS)
    delete_area(mc->mc_areaid);
#endif
#if defined(MM_SHMT_IPCSHM)
    if (fdmem != -1)
        shmctl(fdmem, IPC_RMID, NULL);
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    if (fdsem != -1)
        close(fdsem);
#endif
#if defined(MM_SEMT_IPCSEM)
    if (fdsem != -1)
        semctl(fdsem, 0, IPC_RMID, 0);
    if (fdsem_rd != -1)
        semctl(fdsem_rd, 0, IPC_RMID, 0);
#endif
#if defined(MM_SHMT_MMFILE)
    unlink(fnmem);
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    unlink(fnsem);
#endif
    return NULL;
    END_FAILURE
}

int mm_core_permission(void *core, mode_t mode, uid_t owner, gid_t group)
{
    int rc;
    mem_core *mc;
#if defined(MM_SEMT_IPCSEM)
    union semun ick;
    struct semid_ds buf;
    int sems[2], i;
#endif

    if (core == NULL)
        return -1;
    mc = (mem_core *)((char *)core-SIZEOF_mem_core);
    rc = 0;
#if defined(MM_SHMT_MMFILE)
    if (rc == 0 && chmod(mc->mc_fnmem, mode) < 0)
        rc = -1;
    if (rc == 0 && chown(mc->mc_fnmem, owner, group) < 0)
        rc = -1;
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    if (rc == 0 && chmod(mc->mc_fnsem, mode) < 0)
        rc = -1;
    if (rc == 0 && chown(mc->mc_fnsem, owner, group) < 0)
        rc = -1;
#endif
#if defined(MM_SEMT_IPCSEM)
    if (rc == 0) {
        sems[0] = mc->mc_fdsem;
        sems[1] = mc->mc_fdsem_rd;
        ick.buf = &buf;
        for (i = 0; i < 2; i++) {
            if (semctl(sems[i], 0, IPC_STAT, ick) < 0) {
                rc = -1;
                break;
            }
            if ((int)owner != -1)
                buf.sem_perm.uid = owner;
            if ((int)group != -1)
                buf.sem_perm.gid = group;
            buf.sem_perm.mode = mode;
            if (semctl(sems[i], 0, IPC_SET, ick) < 0) {
                rc = -1;
                break;
            }
        }
    }
#endif
    return rc;
}

void mm_core_delete(void *core)
{
    mem_core *mc;
#if defined(MM_SHMT_IPCSHM)
    int fdmem;
#endif
    int fdsem;
#if defined(MM_SEMT_IPCSEM)
    int fdsem_rd;
#endif
    size_t size;
#if defined(MM_SHMT_MMFILE)
    char fnmem[MM_MAXPATH];
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    char fnsem[MM_MAXPATH];
#endif

    if (core == NULL)
        return;
    mc = (mem_core *)((char *)core-SIZEOF_mem_core);
    size  = mc->mc_size;
#if defined(MM_SHMT_IPCSHM)
    fdmem = mc->mc_fdmem;
#endif
#if !defined(MM_SEMT_FLOCK)
    fdsem = mc->mc_fdsem;
#endif
#if defined(MM_SEMT_IPCSEM)
    fdsem_rd = mc->mc_fdsem_rd;
#endif
#if defined(MM_SEMT_FLOCK)
    fdsem = mm_core_getfdsem(mc);
#endif
#if defined(MM_SHMT_MMFILE)
    memcpy(fnmem, mc->mc_fnmem, MM_MAXPATH);
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    memcpy(fnsem, mc->mc_fnsem, MM_MAXPATH);
#endif

#if defined(MM_SHMT_MMANON) || defined(MM_SHMT_MMPOSX) || defined(MM_SHMT_MMZERO) || defined(MM_SHMT_MMFILE)
    munmap((caddr_t)mc, size);
#endif
#if defined(MM_SHMT_IPCSHM)
    shmdt((void *)mc);
    shmctl(fdmem, IPC_RMID, NULL);
#endif
#if defined(MM_SHMT_MMFILE)
    unlink(fnmem);
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    close(fdsem);
#endif
#if defined(MM_SEMT_FLOCK) || defined(MM_SEMT_FCNTL)
    unlink(fnsem);
#endif
#if defined(MM_SEMT_IPCSEM)
    semctl(fdsem, 0, IPC_RMID, 0);
    semctl(fdsem_rd, 0, IPC_RMID, 0);
#endif
    return;
}

size_t mm_core_size(const void *core)
{
    mem_core *mc;

    if (core == NULL)
        return 0;
    mc = (mem_core *)((char *)core-SIZEOF_mem_core);
    return (mc->mc_usize);
}

int mm_core_lock(const void *core, mm_lock_mode mode)
{
    mem_core *mc;
    int rc = 0;
    int fdsem;

    if (core == NULL)
        return FALSE;
    mc = (mem_core *)((char *)core-SIZEOF_mem_core);
#if !defined(MM_SEMT_FLOCK)
    fdsem = mc->mc_fdsem;
#endif
#if defined(MM_SEMT_FLOCK)
    fdsem = mm_core_getfdsem(mc);
#endif

#if defined(MM_SEMT_FCNTL)
    if (mode == MM_LOCK_RD)
        while (((rc = fcntl(fdsem, F_SETLKW, &mm_core_dolock_rd)) < 0) && (errno == EINTR)) ;
    else
        while (((rc = fcntl(fdsem, F_SETLKW, &mm_core_dolock_rw)) < 0) && (errno == EINTR)) ;
#endif
#if defined(MM_SEMT_FLOCK)
    if (mode == MM_LOCK_RD)
        while (((rc = flock(fdsem, LOCK_SH)) < 0) && (errno == EINTR)) ;
    else
        while (((rc = flock(fdsem, LOCK_EX)) < 0) && (errno == EINTR)) ;
#endif
#if defined(MM_SEMT_IPCSEM)
    if (mode == MM_LOCK_RD) {
        while (((rc = semop(mc->mc_fdsem_rd, mm_core_dolock, 2)) < 0) && (errno == EINTR)) ;
        mc->mc_readers++;
        if (mc->mc_readers == 1)
            while (((rc = semop(fdsem, mm_core_dolock, 2)) < 0) && (errno == EINTR)) ;
        while (((rc = semop(mc->mc_fdsem_rd, mm_core_dounlock, 1)) < 0) && (errno == EINTR)) ;
    }
    else {
        while (((rc = semop(fdsem, mm_core_dolock, 2)) < 0) && (errno == EINTR)) ;
    }
    mc->mc_lockmode = mode;
#endif
#if defined(MM_SEMT_BEOS)
    rc = 0;
    if (atomic_add(&mc->mc_ben, 1) > 0) {
        /* someone already in lock... acquire sem and wait */
        if (acquire_sem(mc->mc_semid) != B_NO_ERROR) {
            atomic_add(&mc->mc_ben, -1);
            rc = -1;
        }
    }
#endif

    if (rc < 0) {
        ERR(MM_ERR_CORE|MM_ERR_SYSTEM, "Failed to lock");
        rc = FALSE;
    }
    else
        rc = TRUE;
    return rc;
}

int mm_core_unlock(const void *core)
{
    mem_core *mc;
    int rc = 0;
    int fdsem;

    if (core == NULL)
        return FALSE;
    mc = (mem_core *)((char *)core-SIZEOF_mem_core);
#if !defined(MM_SEMT_FLOCK)
    fdsem = mc->mc_fdsem;
#endif
#if defined(MM_SEMT_FLOCK)
    fdsem = mm_core_getfdsem(mc);
#endif

#if defined(MM_SEMT_FCNTL)
    while (((rc = fcntl(fdsem, F_SETLKW, &mm_core_dounlock)) < 0) && (errno == EINTR)) ;
#endif
#if defined(MM_SEMT_FLOCK)
    while (((rc = flock(fdsem, LOCK_UN)) < 0) && (errno == EINTR)) ;
#endif
#if defined(MM_SEMT_IPCSEM)
    if (mc->mc_lockmode == MM_LOCK_RD) {
        while (((rc = semop(mc->mc_fdsem_rd, mm_core_dolock, 2)) < 0) && (errno == EINTR)) ;
        mc->mc_readers--;
        if (mc->mc_readers == 0)
            while (((rc = semop(fdsem, mm_core_dounlock, 1)) < 0) && (errno == EINTR)) ;
        while (((rc = semop(mc->mc_fdsem_rd, mm_core_dounlock, 1)) < 0) && (errno == EINTR)) ;
    }
    else {
        while (((rc = semop(fdsem, mm_core_dounlock, 1)) < 0) && (errno == EINTR)) ;
    }
#endif
#if defined(MM_SEMT_BEOS)
    rc = 0;
    if (atomic_add(&mc->mc_ben, -1) > 1)
        release_sem(mc->mc_semid);
#endif

    if (rc < 0) {
        ERR(MM_ERR_CORE|MM_ERR_SYSTEM, "Failed to unlock");
        rc = FALSE;
    }
    else
        rc = TRUE;
    return rc;
}

/*EOF*/
