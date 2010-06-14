/* #############################################################################
 * general functions for all parts of the program
 * #############################################################################
 * Copyright (C) 2005-2009 Harry Brueckner
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contact: Harry Brueckner <harry_b@mm.st>
 *          Muenchener Strasse 12a
 *          85253 Kleinberghofen
 *          Germany
 * #############################################################################
 */

/* #############################################################################
 * includes
 */
#include "cpm.h"
#ifdef HAVE_CRACKLIB
  // big hack because CRACKLIB_DICTPATH is used in crack.h
  #define CLD CRACKLIB_DICTPATH
  #undef CRACKLIB_DICTPATH
  #include <crack.h>
  #define CRACKLIB_DICTPATH CLD
  #undef CLD

  #ifndef CRACKLIB_DICTPATH
    #error CRACKLIB_DICTPATH not defined.
    #define CRACKLIB_DICTPATH ""
  #endif
#endif
#include "configuration.h"
#include "general.h"
#include "memory.h"
#include "string.h"
#ifdef TRACE_DEBUG
  #include <stdarg.h>
#endif


/* #############################################################################
 * internal functions
 */
char* createRealPassword(int length);


/* #############################################################################
 *
 * Description    print debug information to stderr
 * Author         Harry Brueckner
 * Date           2008-09-29
 * Arguments      char* filename    - filename where the trace call is
 *                int line          - line number of tha call
 *                char* fmt         - printf like format string
 *                ...               - arguments to printf
 * Return         void
 */
#ifdef TRACE_DEBUG
void cpm_trace(const char* filename, int line, int level, char* fmt, ...)
  {
    va_list             ap;

    if (!config ||
        config -> debuglevel == 0 ||
        config -> debuglevel < level)
      { return; }

    fprintf(stderr, "[trace] %s, line %d: ", filename, line);
    va_start(ap, fmt);
    /* Flawfinder: ignore */
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }
#endif


/* #############################################################################
 *
 * Description    create a backup file of the given file
 * Author         Harry Brueckner
 * Date           2005-05-13
 * Arguments      char* filename              - filename to create the backup of
 *                SHOWERROR_FN showerror_cb   - error handling function in
 *                                              case of errors
 * Return         1 on error, otherwise 0
 */
int createBackupfile(char* filename, SHOWERROR_FN showerror_cb)
  {
    struct stat         filestat;
    mode_t              mode = 0;
    off_t               size = 0;
    int                 fd;
    char*               buffer = NULL;
    char*               newname;
    char*               tmpbuffer;

    TRACE(99, "createBackupfile()", NULL);

    if (!config -> createbackup)
      {   /* if we don't want backup files at all, we just do nothing*/
        return 0;
      }

    newname = memAlloc(__FILE__, __LINE__,
        strlen(filename) + 2);
    strStrncpy(newname, filename, strlen(filename) + 1);
    strStrncat(newname, "~", 1 + 1);
    if (unlink(newname) &&
        errno != ENOENT)
      {   /* first we try to unlink any backup file; if it doesn't exist it's
           * not an error
           */
        tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
        snprintf(tmpbuffer, STDBUFFERLENGTH,
            _("error %d (%s) removing file '%s'."),
            errno,
            strerror(errno),
            newname);
        showerror_cb(_("file error"), tmpbuffer);
        memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);
      }

    fd = fileLockOpen(filename, O_RDONLY, -1, &tmpbuffer);
    if (fd == -1)
      {
        if (tmpbuffer)
          { memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH); }
      }
    else if (!fstat(fd, &filestat) &&
        filestat.st_size)
      {   /* we found a file and can read it */
        size = filestat.st_size;

        /* we only get the permissions for owner/group/world */
        mode = filestat.st_mode;
        mode &= (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP |
            S_IROTH | S_IWOTH | S_IXOTH);

        buffer = memAlloc(__FILE__, __LINE__, size);

        /* Flawfinder: ignore */
        if (read(fd, buffer, size) != size)
          {
            memFree(__FILE__, __LINE__, buffer, size);
            memFreeString(__FILE__, __LINE__, newname);
            return 1;
          }

        lockf(fd, F_ULOCK, 0L);
        close(fd);
      }

    if (size)
      {   /* we can finally write the backup file */
        /* for the backup file we do not want to follow symlinks but want to
         * create new files and truncate the file before we write to it
         */
        fd = fileLockOpen(newname, O_WRONLY | O_CREAT | O_NOFOLLOW | O_TRUNC,
            mode, &tmpbuffer);
        if (fd == -1)
          {
            showerror_cb(_("file error"), tmpbuffer);
            memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

            memFree(__FILE__, __LINE__, buffer, size);
            memFreeString(__FILE__, __LINE__, newname);

            return 1;
          }

        if (write(fd, buffer, size) != size)
          {   /* error writing the file */
            tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
            snprintf(tmpbuffer, STDBUFFERLENGTH,
                _("error %d (%s) writing file '%s'."),
                errno,
                strerror(errno),
                newname);
            showerror_cb(_("file error"), tmpbuffer);
            memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

            memFree(__FILE__, __LINE__, buffer, size);
            memFreeString(__FILE__, __LINE__, newname);

            lockf(fd, F_ULOCK, 0);
            close(fd);
            return 1;
          }

        lockf(fd, F_ULOCK, 0);
        close(fd);
      }

    memFree(__FILE__, __LINE__, buffer, size);
    memFreeString(__FILE__, __LINE__, newname);

    return 0;
  }


/* #############################################################################
 *
 * Description    create a random password from our configured password alphabet
 *                and only return good passwords (checked via cracklib)
 * Author         Harry Brueckner
 * Date           2005-04-19
 * Arguments      int size          - length of the created password
 * Return         char* containing the password; it must be freed by the caller
 */
#ifdef HAVE_CRACKLIB
char* createPassword(int length)
  {
    int                 i = 0;
    char const*         errormsg;
    char*               dictionary;
    char*               password;

    TRACE(99, "createPassword()", NULL);

    dictionary = memAlloc(__FILE__, __LINE__, strlen(CRACKLIB_DICTPATH) + 1);
    strStrncpy(dictionary, CRACKLIB_DICTPATH, strlen(CRACKLIB_DICTPATH) + 1);

    while (i++ < 10)
      {
        password = createRealPassword(length);
        errormsg = FascistCheck(password, dictionary);

        if (errormsg)
          {
            memFreeString(__FILE__, __LINE__, password);
            password = NULL;
          }
        else
          { break; }
      }

    memFreeString(__FILE__, __LINE__, dictionary);

    return password;
  }
#else
char* createPassword(int length)
  {
    TRACE(99, "createPassword()", NULL);

    return createRealPassword(length);
  }
#endif


/* #############################################################################
 *
 * Description    create a random password from our configured password alphabet
 * Author         Harry Brueckner
 * Date           2005-04-13
 * Arguments      int size  - length of the created password
 * Return         char* containing the password; it must be freed by the caller
 */
char* createRealPassword(int length)
  {
    unsigned int        number;
    int                 alphalength,
                        i,
                        rnd;
    char*               password;

    TRACE(99, "createRealPassword()", NULL);

    /* Flawfinder: ignore */
    rnd = open("/dev/random", O_RDONLY);

    if (rnd == -1)
      { return NULL; }

    alphalength = strlen(config -> passwordalphabet);
    password = memAlloc(__FILE__, __LINE__, length + 1);
    for (i = 0; i < length; i++)
      {
        /* we get an unsigned int from /dev/random */

        /* Flawfinder: ignore */
        if (read(rnd, &number, sizeof(int)) == sizeof(int))
          {   /* and get the corresponding alphabet letter */
            password[i] = config -> passwordalphabet[number % alphalength];
          }
      }
    password[i] = 0;

    close(rnd);

    return password;
  }


/* #############################################################################
 *
 * Description    check if the given file exists
 * Author         Harry Brueckner
 * Date           2005-04-18
 * Arguments      char* filename  - name of the file
 * Return         int 1 if the file exists, otherwise 0
 */
int fileExists(char* filename)
  {
    struct stat         filestat;

    TRACE(99, "fileExists()", NULL);

    if (stat(filename, &filestat))
      { return 0; }
    else
      { return 1; }
  }


/* #############################################################################
 *
 * Description    create a lockfile for the given filename/extension
 * Author         Harry Brueckner
 * Date           2005-05-20
 * Arguments      char* filename  - filename to create a lock for
 *                char* extension - extension to use (without '.')
 *                char** errormsg - errormessage of the open command
 * Return          0 if the lock was successfully created
 *                -1 if the file already exists
 *                 1 if the lock could not be created
 */
int fileLockCreate(char* filename, char* extension, char** errormsg)
  {
    pid_t               pid;
    ssize_t             len,
                        wsize;
    int                 fd,
                        size;
    char*               fullname;
    /* Flawfinder: ignore */
    char                pidstring[32];

    TRACE(99, "fileLockCreate()", NULL);

    /* create the filename for the backup */
    size = strlen(filename) + 1 + strlen(extension) + 1;
    fullname = memAlloc(__FILE__, __LINE__, size);
    strStrncpy(fullname, filename, strlen(filename) + 1);
    strStrncat(fullname, ".", 1 + 1);
    strStrncat(fullname, extension, strlen(extension) + 1);

    /* in case the lockfile already was used, we must free it first */
    if (runtime -> lockfile)
      { memFreeString(__FILE__, __LINE__, runtime -> lockfile); }

    runtime -> lockfile = fullname;

    if (fileExists(runtime -> lockfile))
      { return -1; }

    /* for the lockfile we want to create new files and truncate the file
     * before we write to it and fail if the file already exists;
     * we don't need O_NOFOLLOW because we exit with an error if the file
     * exists
     */
    fd = fileLockOpen(runtime -> lockfile,
        O_WRONLY | O_CREAT | O_TRUNC | O_EXCL,
        S_IRUSR | S_IWUSR, errormsg);

    if (fd == -1)
      {   /* we could not get a lock */
        return 1;
      }
    else
      {   /* lockfile was successfully created */
        pid = getpid();
        snprintf(pidstring, 32, "%d\n", pid);

        len = strlen(pidstring);
        wsize = write(fd, pidstring, len);

        lockf(fd, F_ULOCK, 0L);
        close(fd);

        if (wsize == strlen(pidstring))
          { return 0; }
        else
          { return 1; }
      }
  }


/* #############################################################################
 *
 * Description    open a locked file
 * Author         Harry Brueckner
 * Date           2005-05-20
 * Arguments      char* path      - see open
 *                int flags       - see open
 *                mode_t mode     - see open
 *                char** errormsg - errormessage of the open command
 * Return         the new file descriptor
 */
int fileLockOpen(char* filename, int flags, mode_t mode, char** errormsg)
  {
    int                 fd,
                        lockmode,
                        try = 0;

    TRACE(99, "fileLockOpen()", NULL);

    *errormsg = NULL;

    if (mode == -1)
      {

        /* Flawfinder: ignore */
        fd = open(filename, flags);
      }
    else
      {

        /* Flawfinder: ignore */
        fd = open(filename, flags, mode);
      }

    if (fd == -1)
      {   /* error opening the file */
        *errormsg = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
        snprintf(*errormsg, STDBUFFERLENGTH,
            _("error %d (%s) opening file '%s'."),
            errno,
            strerror(errno),
            filename);

        return -1;
      }

    /* we always want to lock the whole file */
    if (lseek(fd, 0L, SEEK_SET) == -1)
      {
        *errormsg = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
        snprintf(*errormsg, STDBUFFERLENGTH,
            _("error %d (%s) seeking in file '%s'."),
            errno,
            strerror(errno),
            filename);

        close(fd);
        return -1;
      }

    if ((flags & O_WRONLY) ||
        (flags & O_RDWR))
      { lockmode = F_TLOCK; }
    else
      { lockmode = F_TEST; }

    while (lockf(fd, lockmode, 0L) < 0)
      {
        if (errno == EAGAIN)
          {   /* the file already is locked, we try again in a few seconds */
            if (try++ < MAX_LOCKF_TRY)
              {   /* sleep a little while */
                sleep(1);
                continue;
              }

            *errormsg = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
            snprintf(*errormsg, STDBUFFERLENGTH,
                _("could not exclusively open '%s'."),
                filename);

            close(fd);
            return -1;
          }

        *errormsg = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
        snprintf(*errormsg, STDBUFFERLENGTH,
            _("error %d (%s) locking file '%s'."),
            errno,
            strerror(errno),
            filename);

        close(fd);
        return -1;
      }

    return fd;
  }


/* #############################################################################
 *
 * Description    remove the lockfile
 * Author         Harry Brueckner
 * Date           2005-05-20
 * Arguments      char** errormsg - errormessage of the open command
 * Return         0 if unlocking was successful; otherwise 1
 */
int fileLockRemove(char** errormsg)
  {
    TRACE(99, "fileLockRemove()", NULL);

    *errormsg = NULL;

    if (!runtime -> lockfile)
      { return 0; }

    /* now unlink the file */
    if (unlink(runtime -> lockfile) == -1)
      {
        *errormsg = strerror(errno);
        return 1;
      }
    else
      { return 0; }
  }


/* #############################################################################
 *
 * Description    validate the given password and return the cracklib
 *                error message
 * Author         Harry Brueckner
 * Date           2005-04-19
 * Arguments      char* password  - password to verify
 * Return         const char* to the error message, NULL if the password is ok
 */
#ifdef HAVE_CRACKLIB
char* isGoodPassword(char* password)
  {
    char*               dictionary;
    char*               result;

    TRACE(99, "isGoodPassword()", NULL);

    dictionary = memAlloc(__FILE__, __LINE__, strlen(CRACKLIB_DICTPATH) + 1);
    strStrncpy(dictionary, CRACKLIB_DICTPATH, strlen(CRACKLIB_DICTPATH) + 1);

    result = (char*)FascistCheck(password, dictionary);

    memFreeString(__FILE__, __LINE__, dictionary);

    return result;
  }
#else
char* isGoodPassword(char* password)
  {
    TRACE(99, "isGoodPassword()", NULL);

    return NULL;
  }
#endif


/* #############################################################################
 *
 * Description    check if the given file is read-only
 * Author         Harry Brueckner
 * Date           2005-04-13
 * Arguments      char* filename  - filename to check
 * Return         int 1 if it's read-only, otherwise 0
 */
int isReadonly(char* filename)
  {
    struct stat         filestat;
    int                 found,
                        i,
                        ngroups_max,
                        result = 1;
    GETGROUPS_T*        grouplist;
    GETGROUPS_T         egid;
    uid_t               euid;

    TRACE(99, "isReadonly()", NULL);

#ifndef HAVE_GETGROUPS
    /* if getgroups() does not exist, we can't check this */
    return 0;
#endif

    ngroups_max = (int)sysconf(_SC_NGROUPS_MAX);
    grouplist = memAlloc(__FILE__, __LINE__, sizeof(GETGROUPS_T) *
        (ngroups_max + 1));
    found = getgroups(ngroups_max, grouplist);

    /* we get the euid/eguid */
    egid = getegid();
    euid = geteuid();

    if (!stat(filename, &filestat))
      {
        if (filestat.st_mode & S_IWOTH)
          {   /* it's world writable */
            result = 0;
          }
        else if (filestat.st_mode & S_IWGRP &&
            filestat.st_gid == egid)
          {   /* we got group writepermissions */
            result = 0;
          }
        else if (filestat.st_mode & S_IWUSR &&
            filestat.st_uid == euid)
          {   /* we got owner writepermissions */
            result = 0;
          }
        else
          {
            for (i = 0; i < found; i++)
              {
                if (filestat.st_mode & S_IWGRP &&
                    filestat.st_gid == grouplist[i])
                  {   /* we got group writepermissions */
                    result = 0;
                    break;
                  }
              }
          }
      }
    else
      {   /* file does not exist, so we assume write permissions */
        result = 0;
      }

    /* free the groups */
    memFree(__FILE__, __LINE__, grouplist, sizeof(GETGROUPS_T) *
        (ngroups_max + 1));

    return result;
  }


/* #############################################################################
 *
 * Description    resolve a filename to it's physical file in case it is a
 *                sumbolic link
 * Author         Harry Brueckner
 * Date           2005-05-23
 * Arguments      char* filename  - filename to resolve
 * Return         char* pointing to the real filename
 */
char* resolveFilelink(char* filename)
  {
    int                 size;
    char*               newfile;
    char*               tmpbuffer;

    TRACE(99, "resolveFilelink()", NULL);

    tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);

    /* Flawfinder: ignore */
    size = readlink(filename, tmpbuffer, STDBUFFERLENGTH);

    if (size == -1 ||
        size >= STDBUFFERLENGTH)
      {   /* if the buffer is too small, we better stay with the original
           * filename
           */
        size = strlen(filename) + 1;
        newfile = memAlloc(__FILE__, __LINE__, size);
        strStrncpy(newfile, filename, size);

        memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);
        return newfile;
      }

    newfile = memAlloc(__FILE__, __LINE__, size + 1);
    strStrncpy(newfile, tmpbuffer, size + 1);
    memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

    return newfile;
  }


/* #############################################################################
 *
 * Description    we just provide this in case strerror() does not exist
 * Author         Harry Brueckner
 * Date           2005-05-22
 * Arguments      int errnum  - errno value to translatio to a string
 * Return         char* pointing to the static error message string
 */
#ifndef HAVE_STRERROR
char *strerror(int errnum)
  {
    return NULL;
  }
#endif


/* #############################################################################
 */

