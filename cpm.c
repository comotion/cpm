/* #############################################################################
 * program to manage passwords from the commandline
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
#include "configuration.h"
#include "general.h"
#include "gpg.h"
#include <termios.h>
#include "interface_cli.h"
#include "interface_gui.h"
#include "interface_keys.h"
#include "interface_utf8.h"
#include "interface_xml.h"
#include "memory.h"
#include "options.h"
#include "patternparser.h"
#include "resource.h"
#include "security.h"
#include "xml.h"
#include "zlib.h"


/* #############################################################################
 * internal functions
 */
RETSIGTYPE sighandler(int signum);
#ifdef TEST_OPTION
  void testMemset(void);
#endif


/* #############################################################################
 * global variables
 */
#ifdef MANUAL_EXTERN_ENVIRON
char**                  environ;
#endif

static struct termios tcsaved;
void savetermios(void)
{
    tcgetattr(0, &tcsaved);
}
void restoretermios(void)
{
    tcsetattr(0, TCSANOW, &tcsaved);
}

/* #############################################################################
 *
 * Description    main function of the program
 * Author         Harry Brueckner
 * Date           2005-03-16
 * Arguments      int     argc    - argument counter
 *                char**  argv    - commandline arguments
 *                char**  envp    - process environment (only on Solaris)
 * Return         return code of the program
 */
#ifdef HAVE_EXTERN_ENVIRON
/* everything is in place, we have the environ variable */
int main(int argc, char **argv)
#else
#ifdef MANUAL_EXTERN_ENVIRON
/* we can manually declare the variable */
int main(int argc, char **argv)
#else
/* if don't have any envorin variable at all */
int main(int argc, char **argv, char **envp)
#endif
#endif
  {
    rlim_t              memlock_limit = -2;
    int                 error = 0,
                        max_mem_lock = 0,
                        memory_safe = 0,
                        ptrace_safe = 0;
#ifdef TEST_OPTION
    int                 testrun = 0;
#endif
    char*               binaryname;

    savetermios();
    TRACE(99, "main()", NULL);

#ifndef HAVE_EXTERN_ENVIRON
#ifndef MANUAL_EXTERN_ENVIRON
    /* since in solaris environ does not exist, we manually pass it along */
    environ = envp;
#endif
#endif

    if (initSecurity(&max_mem_lock, &memory_safe, &ptrace_safe, &memlock_limit))
      { exit(1); }

    /* we initialize gettext */
    setlocale(LC_ALL, "");
#ifdef TEST_OPTION
    bindtextdomain(PACKAGE_NAME, "./po/");
#else
    bindtextdomain(PACKAGE_NAME, LOCALEDIR);
#endif
    textdomain(PACKAGE_NAME);

#ifndef LIBXML_TREE_ENABLED
    fprintf(stderr, _("Tree support not compiled in to libxml2 %s\n"),
        LIBXML_DOTTED_VERSION);
    exit(1);
#endif

    /*
     * This function installs "sighandler" to handle the SIGINT and returns a
     * pointer to the previously installed handler for this signal (which is
     * the default handler SIG_DFL initially). If we try to install another
     * handler to handle SIGINT at some other time... Then the new handler
     * replaces this current one and returns a pointer to this handler.
     */
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGALRM, sighandler);
    /* the SIGWINCH handler is set in userInterface() */

    initConfiguration();

    runtime -> memlock_limit  = memlock_limit;
    runtime -> max_mem_lock   = max_mem_lock;
    runtime -> memory_safe    = memory_safe;
    runtime -> ptrace_safe    = ptrace_safe;

    initKeys();
    initPatternparser();
    initXML();
    initXMLInterface();

    if (getOptions(argc, argv))
      {
        fprintf(stderr, _("Try `%s --help' for more information.\n"), argv[0]);
        error = 1;
      }
    if (!error && config -> help)
      { showHelp(); }
    else if (!error && config -> version)
      { showVersion(); }
    else if (!error)
      {
        getDefaultOptions();
        if (readResources())
            return 1;

        if (config -> dbfilecmd)
          {   /* the --file option must overwrite the resource file */
            runtime -> dbfile = resolveFilelink(config -> dbfilecmd);
          }
        else
          {   /* we use the resource file configuration or the compiletime
               * default
               */
            runtime -> dbfile = resolveFilelink(config -> dbfilerc);
          }
      }

    /* we switch to read-only mode on request */
    if (config -> readonly)
      { runtime -> readonly = 1; }

    /* in case our basename is cpmv, we switch to read-only mode */
    binaryname = basename(argv[0]);
    if (!strcmp(binaryname, "cpmv"))
      { runtime -> readonly = 1; }

    initGPG();

    if (!error && config -> security)
      { checkSecurity(0); }

#ifdef TEST_OPTION
    if (!error &&
        config -> testrun &&
        !strncmp(config -> testrun, "compress", 8))
      {
        testCompress();
        testrun = 1;
      }
    if (!error &&
        config -> testrun &&
        !strcmp(config -> testrun, "environment"))
      {
        testEnvironment();
        testrun = 1;
      }
    if (!error &&
        config -> testrun && (
        !strcmp(config -> testrun, "backup") ||
        !strcmp(config -> testrun, "garbage") ||
        !strcmp(config -> testrun, "searchpattern")))
      { testrun = 1; }
#endif

    if (config -> configtest &&
        !error)
      { fprintf(stderr, _("configuration ok.\n")); }

    if (config -> environtmentlist &&
        !error)
      { listEnvironment(); }

    if (!error &&
        !config -> configtest &&
        !config -> environtmentlist &&
        !config -> help &&
        !config -> security &&
        !config -> version)
      {
#ifdef TEST_OPTION
        if (checkSecurity(1) != MAX_SECURITY_LEVEL &&
            !config -> testrun)
#else
        if (checkSecurity(1) != MAX_SECURITY_LEVEL)
#endif
          {
            checkSecurity(0);
            printf("\n%s %s\n%s\n",
                _("Maximum security level not reached."),
                _("Your database will be less protected while CPM is running."),
                _("Are you sure you want to continue?"),
                _("Press CTRL+C to stop now or ENTER to continue."));

            fgetc(stdin);
          }
        if (runtime -> guimode)
          {   /* we run in interactive mode */
            
            /* set inactivity timeout */
            alarm(config->inactivetimeout);

            userInterface();
          }
        else
          {   /* we run in CLI mode */
            error = cliInterface();
#ifdef TEST_OPTION
            if (error == 2)
              {   /* for testruns, we must modify the stuff a little */
                error = 0;
                testrun = 1;
              }
#endif
          }
      }

    freeGPG();
    freeXMLInterface();
    freeUTF8Interface();
    freeXML();
    freePatternparser();
    freeKeys();
    freeConfiguration();

    if (memCheck())
      {   /* we validate our memory consumption */
        fprintf(stderr, _("error: memory leak detected.\n"));
        if (memCheck() > 0)
          {
            fprintf(stderr, _("%ld byte of memory were not freed.\n"),
                memCheck());
          }
        else
          {
            fprintf(stderr,
                _("%ld byte of memory were freed without being allocated.\n"),
                memCheck());
          }

        fprintf(stderr, _("Please send a report about this problem to Harry Brueckner <harry_b@mm.st>.\n"));

        error = 1;
      }

#ifdef TEST_OPTION
    if (testrun)
      { return 0; }
    else
      { return error; }
#else
    return error;
#endif
  }


/* #############################################################################
 *
 * Description    signal handler to catch SIGINT and SIGTERM
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      int signum  - signal number
 * Return         void
 */
RETSIGTYPE sighandler(int signum){
    char*               dummy;

    clear_screen();
    switch (signum) {
        case SIGALRM:
            if(runtime->datachanged) {
                fprintf(stderr, "\nInactivity but data changed, not quitting\n");
                return;
            }
        case SIGINT:
        case SIGTERM:
            /* we have to quit right away */

            freeGPG();
            freeXMLInterface();
            freeUTF8Interface();
            freeXML();
            freePatternparser();
            freeKeys();

            if(runtime->lockfilecreated) { fileLockRemove(&dummy); }
            restoretermios();
            freeConfiguration();
            if(signum == SIGALRM)
                fprintf(stderr, "\nCPM: Inactivity timeout\n");
            else
                fprintf(stderr,"\nquitting on signal %d\n", signum);
            _exit(1);
        default:
            fprintf(stderr, "\nreceived unknown signal %d\n", signum);
    }
}


/* #############################################################################
 */

