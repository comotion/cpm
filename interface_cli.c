/* #############################################################################
 * code for handling the CLI interface
 * #############################################################################
 * Copyright (C) 2005, 2006 Harry Brueckner
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
#include <regex.h>
#include <termios.h>
#include "configuration.h"
#include "general.h"
#include "interface_cli.h"
#include "interface_keys.h"
#include "interface_utf8.h"
#include "interface_xml.h"
#include "listhandler.h"
#include "memory.h"
#include "patternparser.h"
#include "string.h"
#include "xml.h"


/* #############################################################################
 * internal functions
 */
void cliEchoOff(void);
void cliEchoOn(void);
const char* cliDialogPassphrase(int retry, char* realm);
void cliShowError(const char* headline, const char* message);
int cliTreeWalk(char** path);
int prepareSearchexpression(void);


/* #############################################################################
 * global interface variables
 */
regex_t                 searchregex;
static struct           termios terminalsettings;
char**                  searchresult = NULL;
char*                   clisearchpattern = NULL;


/* #############################################################################
 *
 * Description    cli dialog to request the passphrase
 * Author         Harry Brueckner
 * Date           2005-04-03
 * Arguments      int retry           - number of this retry
 *                const char* realm   - name of the realm the passphrase is for
 * Return         const char* to the passphrase
 */
const char* cliDialogPassphrase(int retry, char* realm)
  {
    int                 ret,
                        size = 0;

    printf(_("enter your passphrase (try #%d)\n%s\n"),
        retry,
        realm);

    if (strlen(runtime -> passphrase))
      { return runtime -> passphrase; }

    cliEchoOff();
    while ((ret = fgetc(stdin)) && size < PASSPHRASE_LENGTH)
      {
        if ((char)ret == '\n')
          { break; }

        runtime -> passphrase[size++] = (char)ret;
        runtime -> passphrase[size] = 0;
      }
    cliEchoOn();

    return runtime -> passphrase;
  }


/* #############################################################################
 *
 * Description    turn the cli echo off
 * Author         Harry Brueckner
 * Date           2005-04-03
 * Arguments      void
 * Return         void
 */
void cliEchoOff(void)
  {
    struct termios new_settings;

    tcgetattr(0, &terminalsettings);

    new_settings = terminalsettings;
    new_settings.c_lflag &= (~ECHO);

    tcsetattr(0, TCSANOW, &new_settings);
  }


/* #############################################################################
 *
 * Description    turn the cli echo on
 * Author         Harry Brueckner
 * Date           2005-04-03
 * Arguments      void
 * Return         void
 */
void cliEchoOn(void)
  {
    if(terminalsettings.c_lflag){
    tcsetattr(0, TCSANOW, &terminalsettings);
  }
  }


/* #############################################################################
 *
 * Description    cli interface function which handles the search request
 *                on the commandline
 * Author         Harry Brueckner
 * Date           2005-04-06
 * Arguments      void
 * Return         int 0 if all is ok, 1 on error, 2 if a testrun was made
 */
int cliInterface(void)
  {
    int                 error,
                        found = 0,
                        i,
                        size;
    char***             path = NULL;
    char*               errormsg;

    if (!initUTF8Encoding(config -> encoding))
      {
        fprintf(stderr, _("error: %s\n"),
            _("failed to initialize the character encoding."));
        exit(1);
      }

    if (xmlDataFileRead(runtime -> dbfile, &errormsg,
        cliDialogPassphrase, cliShowError))
      {
        fprintf(stderr, _("error: %s\n"), errormsg);
        exit(1);
      }

    error = patternParse();
#ifdef TEST_OPTION
    if (!error &&
        config -> testrun &&
        !strcmp(config -> testrun, "backup"))
      {
        createBackupfile(runtime -> dbfile, cliShowError);
        fprintf(stderr, "backup creation done.\n");
        return 2;
      }
    if (config -> testrun &&
        !strcmp(config -> testrun, "searchpattern"))
      { return 2; }
#endif

    if (error)
        return 1;

#ifdef TEST_OPTION
    if (config -> testrun &&
        !strcmp("encrypt", config -> testrun))
      {
        if (xmlDataFileWrite(runtime -> dbfile, &errormsg,
            cliDialogPassphrase, cliShowError))
          {
            fprintf(stderr, _("error: %s\n"), errormsg);
            exit(1);
          }
        return 2;
      }

    /* we only return 2 if we don't want to test the CLI search */
    if (config -> testrun &&
        strcmp("clisearch", config -> testrun))
      { return 2; }
#endif

    if (!config -> searchdata)
      {
        fprintf(stderr, _("error: no searchpattern given.\n"));
        return 1;
      }

    if (clisearchpattern)
      { memFreeString(__FILE__, __LINE__, clisearchpattern); }

    /* we build the searchpattern string, the user searches for */
    i = size = 0;
    while (config -> searchdata[i])
      { size += strlen(config -> searchdata[i++]) + 1; }
    clisearchpattern = memAlloc(__FILE__, __LINE__, size);
    clisearchpattern[0] = 0;
    i = 0;
    while (config -> searchdata[i])
      {
        if (strlen(clisearchpattern) > 0)
          { strStrncat(clisearchpattern, " ", 1 + 1); }
        strStrncat(clisearchpattern, config -> searchdata[i],
            strlen(config -> searchdata[i]) + 1);
        i++;
      }

    /* we must check if the user owerwrites our configuration */
    if (runtime -> casesensitive == -1)
      { runtime -> casesensitive = config -> casesensitive; }
    if (runtime -> searchtype == SEARCH_UNDEF)
      { runtime -> searchtype = config -> searchtype; }

    /* we initialize the results list */
    searchresult = NULL;

    /* we build our regular expression */
    if (prepareSearchexpression())
      {
        /* we cycle through the list and try to find all matches */
        path = memAlloc(__FILE__, __LINE__, sizeof(char**));
        *path = NULL;
        found = xmlInterfaceTreeWalk(NULL, path, cliTreeWalk);
        memFree(__FILE__, __LINE__, path, sizeof(char**));

        if (runtime -> searchtype == SEARCH_REGEX)
          { regfree(&searchregex); }

        error = 0;
      }
    else
      { error = 1; }


    if (clisearchpattern)
      { memFreeString(__FILE__, __LINE__, clisearchpattern); }

    if (!error)
      {
        if (found)
          {   /* we have something to display */
            listSort(searchresult);
            for (i = 0; i < found; i++)
              {
                printf("%s\n", searchresult[i]);
              }
          }
        switch (found)
          {
            case 0:
                printf(_("no match found.\n"));
                break;
            case 1:
                printf(_("1 match found.\n"));
                break;
            default:
                printf(_("%d matches found.\n"), found);
                break;
          }
      }

    /* we clean up the results */
    searchresult = listFree(searchresult);

    return 0;
  }


/* #############################################################################
 *
 * Description    show a error message to the user
 * Author         Harry Brueckner
 * Date           2005-04-07
 * Arguments      char* message   - error message to show
 * Return         void
 */
void cliShowError(const char* headline, const char* message)
  {
    fprintf(stderr, _("error: %s %s\n"), headline, message);
  }


/* #############################################################################
 *
 * Description    callback function for xml tree walking; in this function
 *                we search for any matches
 * Author         Harry Brueckner
 * Date           2005-04-06
 * Arguments      char** path   - path we are currently at
 * Return         int number of found matches
 */
int cliTreeWalk(char** path)
  {
    int                 cmp,
                        i = 0,
                        l,
                        lexist,
                        lsize,
                        found = 0;
    char**              pattern = runtime -> searchpatterns;
    char*               cstring = NULL;
    char*               cresult = NULL;

    while (pattern && pattern[i])
      {
        cstring = NULL;
        if (!getPatternSearchString(i, path, &cstring))
          {
            if (runtime -> searchtype == SEARCH_REGEX)
              {   /* regular expression search */
                cmp = regexec(&searchregex, cstring, 0, NULL, 0);
              }
            else
              {   /* regular search */
                if (runtime -> casesensitive)
                  {   /* case sensitive comparison */
                    cmp = strcmp(cstring, clisearchpattern);
                  }
                else
                  {   /* case insensitive comparison */
                    cmp = strcasecmp(cstring, clisearchpattern);
                  }
              }

            if (!cmp)
              {   /* we found a match! */
                cresult = NULL;
                if (!getPatternResultString(i, path, &cresult))
                  {   /* and even have a result to display */
                    lsize = listCount(searchresult);
                    for (l = lexist = 0; l < lsize; l++)
                      {
                        if (!strcmp(searchresult[l], cresult))
                          {   /* this result already exists, nothing to do */
                            lexist = 1;
                            break;
                          }
                      }

                    if (!lexist)
                      {   /* only if we don't know this result yet, we add it */
                        searchresult = listAdd(searchresult, cresult);
                        found++;
                      }
                  }
                if (cresult)
                  { memFreeString(__FILE__, __LINE__, cresult); }
              }
          }
        if (cstring)
          { memFreeString(__FILE__, __LINE__, cstring); }

        i++;
      }

    return found;
  }


/* #############################################################################
 *
 * Description    compile the regular expression for the search
 * Author         Harry Brueckner
 * Date           2005-08-05
 * Arguments      void
 * Return         1 if all is ok, otherwise 0 on error
 */
int prepareSearchexpression(void)
  {
    int                 eflags,
                        result;
    char*               errormsg;

    if (runtime -> searchtype == SEARCH_REGULAR)
      {   /* for the regular search we don't need anything */
        return 1;
      }

    eflags = REG_EXTENDED | REG_NOSUB;

    if (!runtime -> casesensitive)
      { eflags |= REG_ICASE; }

    result = regcomp(&searchregex, clisearchpattern, eflags);
    if (result)
      {
        errormsg = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
        regerror(result, &searchregex, errormsg, STDBUFFERLENGTH);
        fprintf(stderr, "regular expression error: %s\n", errormsg);
        memFree(__FILE__, __LINE__, errormsg, STDBUFFERLENGTH);
      }

    return !result;
  }


/* #############################################################################
 */

