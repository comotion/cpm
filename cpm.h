/* #############################################################################
 * header information for cpm.c
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
#ifndef CPM_H
#define CPM_H


/* #############################################################################
 * global includes
 */
#include "config.h"
#define _GNU_SOURCE

#ifdef HAVE_STDLIB_H
  #ifndef _SVID_SOURCE
   #define _SVID_SOURCE
  #endif
  #include <stdlib.h>
#endif
#include <errno.h>
#ifdef HAVE_FCNTL_H
  #include <fcntl.h>
#endif
#include <sys/ioctl.h>
#ifdef HAVE_LIBINTL_H
  #include <libintl.h>
#endif
#ifdef HAVE_LOCALE_H
  #include <locale.h>
#endif
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#ifdef HAVE_STRINGS_H
  #include <string.h>
#endif
#ifdef HAVE_SYS_FSUID_H
  #include <sys/fsuid.h>
#endif
#include <sys/mman.h>
#include <sys/resource.h>
#ifdef HAVE_SYS_STAT_H
  #include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
  #include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif

#ifndef NO_CRACKLIB
  #define HAVE_CRACKLIB
#endif


/* #############################################################################
 * global variables
 */
#ifndef HAVE_EXTERN_ENVIRON
#ifndef MANUAL_EXTERN_ENVIRON
    /* since in solaris environ does not exist, we declare it ourselves */
extern char**           environ;
#endif
#endif


/* #############################################################################
 * default configuration of cpm
 */

/* the default database file
 */
#define DEFAULT_DB_FILE ".cpmdb"
/* the default resource file and path; the path is used, if the file is not
 * found in the users home directory
 */
#define DEFAULT_RC_PATH_1 "/etc/cpm"
#define DEFAULT_RC_PATH_2 "/etc"
#define DEFAULT_RC_FILE ".cpmrc"
#define DEFAULT_ETC_RC_FILE "cpmrc"

/* this defines the default encoding we use */
#define DEFAULT_ENCODING "ISO-8859-1"

/* how many times we try to get a file lock */
#define MAX_LOCKF_TRY   5


/* macros for maximum and minimum finding */
#define max(x, y)       ((x > y) ? x : y)
#define min(x, y)       ((x < y) ? x : y)

/* our global type of error callback functions, depending on the interface */
typedef void (*SHOWERROR_FN) (const char* headline, const char* errormsg);


/* default size of standard buffers */
#define STDBUFFERLENGTH 512
/* maximum length of the strings we allow through user input (resource files,
 * on the commandline or in the XML file)
 */
#define STDSTRINGLENGTH 256


#ifdef HAVE_LIBINTL_H
  /* we have the real gettext */
  #define _(String) gettext (String)
  #define gettext_noop(String) String
  #define N_(String) gettext_noop (String)
#else
  /* we just fake gettext */
  #define _(String) (String)
  #define N_(String) String
  #define textdomain(Domain)
  #define bindtextdomain(Package, Directory)
#endif


#endif


/* #############################################################################
 */

