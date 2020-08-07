/* #############################################################################
 * header information for configuration.c
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
#ifndef CPM_CONFIGURATION_H
#define CPM_CONFIGURATION_H

/* #############################################################################
 * prototypes
 */
void clearPassphrase(int final);
void freeConfiguration(void);
void initConfiguration(void);


/* #############################################################################
 * global structures
 */
#define PASSPHRASE_LENGTH     256
typedef struct
  {
    char**              defaultkeys;
    char**              defaulttemplates;
    char**              defaulttemplatestatus;
    char**              searchdata;
    char*               dbfilerc;
    char*               dbfilecmd;
    char*               rcfile;
    char*               encoding;
    char*               passwordalphabet;
#ifdef TEST_OPTION
    char*               testrun;
#endif
    char                hidecharacter;

    int                 asktoquit;
    int                 casesensitive;
    int                 compression;
    int                 configtest;
    int                 cracklibstatus;
    int                 createbackup;
    int                 debuglevel;
    int                 encryptdata;
    int                 environtmentlist;
    int                 help;
    int                 infoheight;
    int                 keeppassphrase;
    int                 passwordlength;
    int                 readonly;
    int                 searchtype;
    int                 security;
    int                 templatelock;
    int                 version;
    unsigned int        inactivetimeout;
  } cpmconfig_t;

typedef struct
  {
    rlim_t              memlock_limit;
    char**              resultpatterns;
    char**              searchpatterns;
    char*               dbfile;
    char*               lockfile;
    char*               realm;
    char*               realmhint;
    /* Flawfinder: ignore */
    char                passphrase[PASSPHRASE_LENGTH + 1];

    int                 casesensitive;
    int                 commandlinekeys;
    int                 datachanged;
    int                 guimode;
    int                 lockfilecreated;
    int                 max_mem_lock;
    int                 memory_safe;
    int                 ptrace_safe;
    int                 readonly;
    int                 searchtype;
    int                 updatestatus;
  } cpmruntime_t;


/* #############################################################################
 * global variables
 */
extern cpmconfig_t*            config;
extern cpmruntime_t*           runtime;

#define CRACKLIB_OFF    0
#define CRACKLIB_ON     1

#define SEARCH_UNDEF    0
#define SEARCH_REGEX    1
#define SEARCH_REGULAR  2

#endif


/* #############################################################################
 */

