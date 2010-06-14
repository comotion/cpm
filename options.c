/* #############################################################################
 * code for all commandline argument handling
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
#ifdef __linux__
  #include <bits/wordsize.h>
#endif
#ifdef HAVE_GETOPT_H
  #include <getopt.h>
#endif
#ifdef HAVE_LIBNCURSES
  #include <ncurses.h>
#endif
#ifdef HAVE_LIBXML2
  #include <libxml/xmlversion.h>
#endif
#ifdef HAVE_LIBZ
  #include <zlib.h>
#endif
#include <gpgme.h>
#include "configuration.h"
#include "general.h"
#include "listhandler.h"
#include "memory.h"
#include "options.h"
#include "string.h"


/* #############################################################################
 *
 * Description    in case getOptions() did not provide any values, this
 *                function makes sure, all mandatory values are present
 * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      void
 * Return         void
 */
void getDefaultOptions(void)
  {
    int                 found = 0;
    char*               home;

    TRACE(99, "getDefaultOptions()", NULL);

    /* Flawfinder: ignore */
    home = getenv("HOME");

    if (!config -> dbfilerc)
      {   /* we initialize the default db file */
        config -> dbfilerc = getFilePath(home, DEFAULT_DB_FILE);
      }

    if (!config -> rcfile)
      {   /* we initialize the default rc file */
        config -> rcfile = getFilePath(home, DEFAULT_RC_FILE);

        if (!fileExists(config -> rcfile))
          {   /* we don't have the user's rc file in ~/.cpmrc, so we try to
               * find it in the default RC path (/etc/cpmrc)
               */
            memFreeString(__FILE__, __LINE__, config -> rcfile);
            config -> rcfile = getFilePath(DEFAULT_RC_PATH_1,
                DEFAULT_ETC_RC_FILE);
          }
        else
          { found = 1; }
        if (!found && !fileExists(config -> rcfile))
          {   /* we didn't find it in /etc/cpmrc either, so we try
               * /etc/cpm/cpmrc
               */
            memFreeString(__FILE__, __LINE__, config -> rcfile);
            config -> rcfile = getFilePath(DEFAULT_RC_PATH_2,
                DEFAULT_ETC_RC_FILE);
          }
        else
          { found = 1; }
        if (!found && !fileExists(config -> rcfile))
          {   /* if the file still does not exist, we empty the configuration */
            memFreeString(__FILE__, __LINE__, config -> rcfile);
            config -> rcfile = NULL;
          }
      }
    if (!config -> encoding)
      {
        config -> encoding = memAlloc(__FILE__, __LINE__,
            strlen(DEFAULT_ENCODING) + 1);
        strStrncpy(config -> encoding, DEFAULT_ENCODING,
            strlen(DEFAULT_ENCODING) + 1);
      }
  }


/* #############################################################################
 *
 * Description    this function concats a path and a filename, calculating
 *                its length and allocating memory accordingly
 * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      char* path      - path to use (can be NULL)
 *                char* filename  - filename to append to the path
 * Return         char* pointer to the allocated area
 */
char* getFilePath(char* path, char* filename)
  {
    char*               result;

    TRACE(99, "getFilePath()", NULL);

    if (path)
      {   /* we got a path and a filename */
        result = memAlloc(__FILE__, __LINE__,
            strlen(path) + 1 + strlen(filename) + 1);
        strStrncpy(result, path, strlen(path) + 1);
        strStrncat(result, "/",  1 + 1);
        strStrncat(result, filename, strlen(filename) + 1);
      }
    else
      {   /* we just got a filename */
        result = memAlloc(__FILE__, __LINE__, strlen(filename) + 1);
        strStrncpy(result, filename, strlen(filename) + 1);
      }

    return result;
  }


/* #############################################################################
 *
 * Description    get the commandline options, the command was called with
 * Author         Harry Brueckner
 * Date           2005-03-16
 * Arguments      int     argc    - argument counter
 *                char**  argv    - commandline arguments
 * Return         0 if everything is ok, otherwise 1
 */
int getOptions(int argc, char **argv)
  {
    int                 error = 0;

    TRACE(99, "getOptions()", NULL);

    while (1)
      {
        int             code;
        int             optid = 0;

        /* remember to fix the wrapper script as well if those options are
         * modified.
         */
        static struct option long_options[] =
          {
            { "config",       required_argument,  0, 0 },   /*  0 */
            { "configtest",   no_argument,        0, 0 },   /*  1 */
            { "debuglevel",   optional_argument,  0, 0 },   /*  2 */
            { "encoding",     required_argument,  0, 0 },   /*  3 */
            { "environment",  no_argument,        0, 0 },   /*  4 */
            { "file",         required_argument,  0, 0 },   /*  5 */
            { "help",         no_argument,        0, 0 },   /*  6 */
            { "ignore",       no_argument,        0, 0 },   /*  7 */
            { "key",          required_argument,  0, 0 },   /*  8 */
            { "noencryption", no_argument,        0, 0 },   /*  9 */
            { "noignore",     no_argument,        0, 0 },   /* 10 */
            { "readonly",     no_argument,        0, 0 },   /* 11 */
            { "regex",        no_argument,        0, 0 },   /* 12 */
            { "regular",      no_argument,        0, 0 },   /* 13 */
            { "security",     no_argument,        0, 0 },   /* 14 */
            { "testrun",      optional_argument,  0, 0 },   /* 15 */
            { "version",      no_argument,        0, 0 },   /* 16 */
            { 0,              0,                  0, 0 }
          };

        /* Flawfinder: ignore */
        code = getopt_long(argc, argv, "c:e:f:hirs", long_options, &optid);
        if (code == -1)
        { break; }

        if (code == 0)
          {   /* this is a long option */
            switch (optid)
              {
                case 0:   /* config */
                    code = 'c';
                    break;
                case 1:   /* configtest */
                    config -> configtest = 1;
                    break;
                case 2:   /* debuglevel */
                    code = 'd';
                    break;
                case 3:   /* encoding */
                    code = 'e';
                    break;
                case 4:   /* environment */
                    config -> environtmentlist = 1;
                    break;
                case 5:   /* file */
                    code = 'f';
                    break;
                case 6:   /* help */
                    code = 'h';
                    break;
                case 7:   /* ignore */
                    code = 'i';
                    break;
                case 8:   /* key */
                    if (!runtime -> commandlinekeys)
                      {   /* if we find the first key on the commandline, we
                           * free the list and start collecting those keys
                           */
                        listFree(config -> defaultkeys);
                        runtime -> commandlinekeys = 1;
                      }
                    config -> defaultkeys = listAdd(config -> defaultkeys,
                        optarg);
                    break;
                case 9:   /* noencryption */
                    config -> encryptdata = 0;
                    break;
                case 10:   /* noignore */
                    runtime -> casesensitive = 1;
                    break;
                case 11:   /* readonly */
                    config -> readonly = 1;
                    break;
                case 12:   /* regex */
                    code = 'r';
                    break;
                case 13:   /* regular */
                    runtime -> searchtype = SEARCH_REGULAR;
                    break;
                case 14:   /* security */
                    code = 's';
                    break;
                case 16:   /* version */
                    config -> version = 1;
                    break;
                case 15:   /* testrun */
#ifdef TEST_OPTION
                    if (!optarg)
                      {
                        fprintf(stderr,
                            _("error: no argument given for --testrun.\n"));
                        error = 1;
                      }
                    else
                      {
                        if (strlen(optarg) > STDSTRINGLENGTH)
                          {
                            fprintf(stderr,
                                _("error: --testrun argument too long.\n"));
                            error = 1;
                          }
                        else
                          {
                            config -> testrun = memAlloc(__FILE__, __LINE__,
                                strlen(optarg) + 1);
                            strStrncpy(config -> testrun, optarg,
                                strlen(optarg) + 1);
                          }
                      }
#else
                    fprintf(stderr,
                        _("error: the program was not compiled with the -DTEST_OPTION option.\n"));
                    error = 1;
#endif
                    break;
                default:
                    printf(
                        _("error: getopt returned unknown long option %d.\n"),
                        optid);
              }
          }

        switch (code)
          {
            case 0:   /* long argument only */
                break;
            case 'c':
                if (!optarg)
                  {
                    fprintf(stderr,
                        _("error: no filename given for --config.\n"));
                    error = 1;
                  }
                else if (strlen(optarg) > STDSTRINGLENGTH)
                  {
                    fprintf(stderr,
                        _("error: --config argument too long.\n"));
                    error = 1;
                  }
                else
                  {
                    config -> rcfile = memAlloc(__FILE__, __LINE__,
                        strlen(optarg) + 1);
                    strStrncpy(config -> rcfile, optarg, strlen(optarg) + 1);
                  }
                break;
            case 'd':
                if (!optarg)
                  { config -> debuglevel = 1; }
                else if (strlen(optarg) > 3)
                  {
                    fprintf(stderr,
                        _("error: --debuglevel argument too long.\n"));
                    error = 1;
                  }
                else
                  {
                    /* Flawfinder: ignore */
                    config -> debuglevel = atoi(optarg);
                    if (config -> debuglevel < 0 ||
                        config -> debuglevel > 999)
                      {
                        fprintf(stderr,
                            _("error: --debuglevel must be 0-999.\n"));
                        error = 1;
                      }
                  }
                break;
            case 'e':
                if (!optarg)
                  {
                    fprintf(stderr,
                        _("error: no argument given for --encoding.\n"));
                    error = 1;
                  }
                else if (strlen(optarg) > STDSTRINGLENGTH)
                  {
                    fprintf(stderr,
                        _("error: --encoding argument too long.\n"));
                    error = 1;
                  }
                else
                  {
                    config -> encoding = memAlloc(__FILE__, __LINE__,
                        strlen(optarg) + 1);
                    strStrncpy(config -> encoding, optarg, strlen(optarg) + 1);
                  }
                break;
            case 'f':
                if (!optarg)
                  {
                    fprintf(stderr,
                        _("error: no filename given for --file.\n"));
                    error = 1;
                  }
                else if (strlen(optarg) > STDSTRINGLENGTH)
                  {
                    fprintf(stderr,
                        _("error: --file argument too long.\n"));
                    error = 1;
                  }
                else
                  {
                    config -> dbfilecmd = memAlloc(__FILE__, __LINE__,
                        strlen(optarg) + 1);
                    strStrncpy(config -> dbfilecmd, optarg, strlen(optarg) + 1);
                  }
                break;
            case 'h':
                config -> help = 1;
                break;
            case 'i':
                runtime -> casesensitive = 0;
                break;
            case 'r':
                runtime -> searchtype = SEARCH_REGEX;
                break;
            case 's':
                config -> security = 1;
                break;
            case ':':
            case '?':
                error = 1;
                break;
            default:
                fprintf(stderr,
                    _("error: getopt returned character code 0x%x\n"),
                    code);
                error = 1;
          }
      }

    if (optind < argc)
      {
        int             entries,
                        i;

        entries = argc - optind;
        for (i = 0; i < entries; i++, optind++)
          {
            if (strlen(argv[optind]) > STDSTRINGLENGTH)
              {
                fprintf(stderr, _("error: argument too long.\n"));
                error = 1;
              }
            else
              {
                config -> searchdata = listAdd(config -> searchdata,
                    argv[optind]);
              }
          }
      }

    /* find out wether we run in CLI or GUI mode */
    if (config -> searchdata ||
#ifdef TEST_OPTION
        config -> testrun)
#else
        0)
#endif
      { runtime -> guimode = 0; }
    else
      { runtime -> guimode = 1; }

    return error;
  }


/* #############################################################################
 *
 * Description    show the syntax help for the command
 * Author         Harry Brueckner
 * Date           2005-03-16
 * Arguments      void
 * Return         void
 */
void showHelp(void)
  {
    TRACE(99, "showHelp()", NULL);

    printf(_("usage: cpm [--config FILE] [--help] [PATH] ...\n"));
    printf(_("    --config, -c    configuration file to use [~/%s]\n"),
        DEFAULT_RC_FILE);
    printf(_("    --configtest    verify the configuration file and exit\n"));
#ifdef TRACE_DEBUG
    printf(_("    --debuglevel    debuglevel (0=off, 1 - 999)\n"));
#endif
    printf(_("    --encoding, -e  the encoding in which keyboard input arrives [%s]\n"),
        DEFAULT_ENCODING);
    printf(_("    --environment   list the environment after cleanup\n"));
    printf(_("    --file, -f      database file to use [~/%s]\n"),
        DEFAULT_DB_FILE);
    printf(_("    --help, -h      display this help\n"));
    printf(_("    --ignore, -i    search case insensitive in cli mode\n"));
    printf(_("    --key           overwrite the default encryption keys and use this key\n"));
    printf(_("                    instead; repeat for several keys\n"));
    printf(_("    --noencryption  turn off file encryption\n"));
    printf(_("                    WARNING: THIS IS FOR DEVELOPMENT AND TESTING ONLY!\n"));
    printf(_("    --noignore      search case sensitive in cli mode\n"));
    printf(_("    --readonly      open the database in read-only mode\n"));
    printf(_("    --regex, -r     search with regular expressions in cli mode\n"));
    printf(_("    --regular       use regular search in cli mode\n"));
    printf(_("    --security, -s  run a security check and show the current security status\n"));
#ifdef TEST_OPTION
    printf(_("    --testrun       run one of the testmodes\n"));
    printf(_("                    backup        - run test on the backupfile creation\n"));
    printf(_("                    compress[1-6] - run compression test 1-6\n"));
    printf(_("                    decrypt       - run test on the decryption code\n"));
    printf(_("                    encrypt       - run test on the encryption code\n"));
    printf(_("                    environment   - run test on the environment validation\n"));
    printf(_("                    garbage       - run test on garbage input files\n"));
    printf(_("                    searchpattern - run test on the search patterns\n"));
#endif
    printf(_("    --version       display the version and exit\n"));
    printf(_("    PATH            path to display the password for\n"));
  }


/* #############################################################################
 *
 * Description    show the syntax help for the command
 * Author         Harry Brueckner
 * Date           2005-03-16
 * Arguments      void
 * Return         void
 */
void showVersion(void)
  {
    TRACE(99, "showVersion()", NULL);

#ifdef __linux__
/* we just want to make 32- and 64-bit systems visible */
#ifdef __WORDSIZE
    #define CPM_WORDSIZE __WORDSIZE
#elif _MIPS_SZPTR
    #define CPM_WORDSIZE _MIPS_SZPTR
#elif _LP64
    #define CPM_WORDSIZE 64
#else
    #define CPM_WORDSIZE 32
#endif

    printf("%s (%d bit)\n", PACKAGE_STRING, CPM_WORDSIZE);
#elif __sun__
    printf("%s (solaris)\n", PACKAGE_STRING);
#else
    printf("%s (unknown)\n", PACKAGE_STRING);
#endif
#ifdef CDK_VERSION_H
    printf(_("CDK version %s.%s (%s).\n"),
        CDK_VERSION_MAJOR,
        CDK_VERSION_MINOR,
        CDK_VERSION_PATCH);
#else
    printf(_("CDK version 4 (unknown version).\n"));
#endif
#ifdef GPGME_HAS_RECIPIENT
    printf(_("GpgME version %s (rcpt).\n"), GPGME_VERSION);
#else
    printf(_("GpgME version %s.\n"), GPGME_VERSION);
#endif
#ifdef HAVE_LIBNCURSESW
    printf(_("ncursesw version %s (%d).\n"),
        NCURSES_VERSION, NCURSES_VERSION_PATCH);
#endif
#ifdef HAVE_LIBNCURSES
    printf(_("ncurses version %s (%d).\n"),
        NCURSES_VERSION, NCURSES_VERSION_PATCH);
#endif
    printf(_("XML2 version %s.\n"), LIBXML_DOTTED_VERSION);
    printf(_("zlib version %s.\n"), ZLIB_VERSION);
#ifdef HAVE_CRACKLIB
    printf(_("cracklib is enabled.\n"));
#else
    printf(_("cracklib is disabled.\n"));
#endif
    printf(_("Written by Harry Brueckner <harry_b@mm.st>.\n"));

#undef CPM_WORDSIZE
  }


/* #############################################################################
 */

