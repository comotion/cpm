/* #############################################################################
 * code for the resource handling which is done by the dot conf library
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
#ifdef HAVE_LIBDOTCONF
  #include <dotconf.h>
#endif
#ifdef HAVE_LIBZ
  #include <zlib.h>
#endif
#include "configuration.h"
#include "listhandler.h"
#include "memory.h"
#include "resource.h"
#include "string.h"


/* #############################################################################
 * internal functions
 */
static DOTCONF_CB(cbFlagArgument);
static DOTCONF_CB(cbIntArgument);
static DOTCONF_CB(cbListArgument);
static DOTCONF_CB(cbStringArgument);
FUNC_ERRORHANDLER(errorHandler);


/* #############################################################################
 * global variables
 */
static const configoption_t options[] =
  {
    { "AskToQuit",          ARG_TOGGLE, cbFlagArgument, NULL, CTX_ALL },
    { "CrackLibCheck",      ARG_TOGGLE, cbFlagArgument, NULL, CTX_ALL },
    { "CreateBackup",       ARG_TOGGLE, cbFlagArgument, NULL, CTX_ALL },
    { "KeepPassphrase",     ARG_TOGGLE, cbFlagArgument, NULL, CTX_ALL },
    { "MatchCaseSensitive", ARG_TOGGLE, cbFlagArgument, NULL, CTX_ALL },
    { "TemplateLock",       ARG_TOGGLE, cbFlagArgument, NULL, CTX_ALL },

    { "Compression",        ARG_INT, cbIntArgument, NULL, CTX_ALL },
    { "InfoboxHeight",      ARG_INT, cbIntArgument, NULL, CTX_ALL },
    { "PasswordLength",     ARG_INT, cbIntArgument, NULL, CTX_ALL },

    { "DatabaseFile",       ARG_STR, cbStringArgument, NULL, CTX_ALL },
    { "EncryptionKey",      ARG_STR, cbStringArgument, NULL, CTX_ALL },
    { "HideCharacter",      ARG_STR, cbStringArgument, NULL, CTX_ALL },
    { "PasswordAlphabet",   ARG_STR, cbStringArgument, NULL, CTX_ALL },
    { "SearchType",         ARG_STR, cbStringArgument, NULL, CTX_ALL },

    { "SearchPattern",      ARG_LIST, cbListArgument, NULL, CTX_ALL },
    { "TemplateName",       ARG_LIST, cbListArgument, NULL, CTX_ALL },
    LAST_OPTION
  };
static int              configerror = 0;


/* #############################################################################
 *
 * Description    handle all flag arguments
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      DOTCONF callback
 * Return         NULL
 */
static DOTCONF_CB(cbFlagArgument)
  {
    if (!strcmp(cmd -> name, "AskToQuit"))
      { config -> asktoquit = cmd -> data.value; }
    else if (!strcmp(cmd -> name, "CrackLibCheck"))
      { config -> cracklibstatus = cmd -> data.value; }
    else if (!strcmp(cmd -> name, "CreateBackup"))
      { config -> createbackup = cmd -> data.value; }
    else if (!strcmp(cmd -> name, "KeepPassphrase"))
      { config -> keeppassphrase = cmd -> data.value; }
    else if (!strcmp(cmd -> name, "MatchCaseSensitive"))
      { config -> casesensitive = cmd -> data.value; }
    else if (!strcmp(cmd -> name, "TemplateLock"))
      { config -> templatelock = cmd -> data.value; }

    return NULL;
  }


/* #############################################################################
 *
 * Description    handle all int arguments
 * Author         Harry Brueckner
 * Date           2005-04-05
 * Arguments      DOTCONF callback
 * Return         NULL
 */
static DOTCONF_CB(cbIntArgument)
  {
    if (!strcmp(cmd -> name, "Compression"))
      {
        if (cmd -> data.value >= Z_NO_COMPRESSION &&
            cmd -> data.value <= Z_BEST_COMPRESSION)
          { config -> compression = cmd -> data.value; }
        else
          { config -> compression = Z_BEST_COMPRESSION; }
      }
    else if (!strcmp(cmd -> name, "InfoboxHeight"))
      {
        if (cmd -> data.value < 5)
          { config -> infoheight = 5; }
        else if (cmd -> data.value > 25)
          { config -> infoheight = 25; }
        else
          { config -> infoheight = cmd -> data.value; }
      }
    else if (!strcmp(cmd -> name, "PasswordLength"))
      {
        if (cmd -> data.value > 5)
          { config -> passwordlength = cmd -> data.value; }
        else
          { return _("PasswordLength must be at least be 5."); }
      }

    return NULL;
  }


/* #############################################################################
 *
 * Description    handle all list arguments
 * Author         Harry Brueckner
 * Date           2005-04-08
 * Arguments      DOTCONF callback
 * Return         NULL
 */
static DOTCONF_CB(cbListArgument)
  {
    char*               status;

    if (!strcmp(cmd -> name, "SearchPattern"))
      {   /* define the search patterns */
        if (cmd -> arg_count != 2)
          { return _("SearchPattern always needs two arguments."); }
        if (strlen(cmd -> data.list[0]) > STDSTRINGLENGTH)
          { return _("list name too long."); }
        if (strlen(cmd -> data.list[1]) > STDSTRINGLENGTH)
          { return _("list argument too long."); }

        runtime -> searchpatterns = listAdd(runtime -> searchpatterns,
            cmd -> data.list[0]);
        runtime -> resultpatterns = listAdd(runtime -> resultpatterns,
            cmd -> data.list[1]);
      }
    else if (!strcmp(cmd -> name, "TemplateName"))
      {   /* define default template names */
        if (cmd -> arg_count == 1)
          { status = "normal"; }
        else if (cmd -> arg_count == 2)
          {
            if (!strcmp("normal", cmd -> data.list[1]) ||
                !strcmp("password", cmd -> data.list[1]))
              { status = cmd -> data.list[1]; }
            else
              {
                return _("The TemplateName attribute can only be 'normal' or 'password'.");
              }
          }
        else
          { return _("TemplateName can just have one or two arguments."); }

        if (strlen(cmd -> data.list[0]) > STDSTRINGLENGTH)
          { return _("list name too long."); }
        if (strlen(status) > STDSTRINGLENGTH)
          { return _("list argument too long."); }

        config -> defaulttemplates = listAdd(config -> defaulttemplates,
            cmd -> data.list[0]);
        config -> defaulttemplatestatus = listAdd(
            config -> defaulttemplatestatus,
            status);
      }

    return NULL;
  }


/* #############################################################################
 *
 * Description    handle all string arguments
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      DOTCONF callback
 * Return         NULL
 */
static DOTCONF_CB(cbStringArgument)
  {
    if (strlen(cmd -> data.str) > STDSTRINGLENGTH)
      { return _("string argument too long."); }
    else if (!strcmp(cmd -> name, "DatabaseFile"))
      {
        memFreeString(__FILE__, __LINE__, config -> dbfilerc);
        config -> dbfilerc = memAlloc(__FILE__, __LINE__,
            strlen(cmd -> data.str) + 1);
        strStrncpy(config -> dbfilerc, cmd -> data.str,
            strlen(cmd -> data.str) + 1);
      }
    else if (!strcmp(cmd -> name, "EncryptionKey"))
      {   /* define default encryption keys */
        if (!runtime -> commandlinekeys)
          {   /* we only read the keys if there are none on the command line */
            config -> defaultkeys = listAdd(config -> defaultkeys,
                cmd -> data.str);
          }
      }
    else if (!strcmp(cmd -> name, "HideCharacter"))
      {   /* define the character to show in the passphrase window */
        if (strlen(cmd -> data.str))
            config -> hidecharacter = cmd -> data.str[0];
      }
    else if (!strcmp(cmd -> name, "PasswordAlphabet"))
      {   /* define the alphabet of our created passwords */
        if (strlen(cmd -> data.str))
          {
            config -> passwordalphabet = memAlloc(__FILE__, __LINE__,
                strlen(cmd -> data.str) + 1);
            strStrncpy(config -> passwordalphabet, cmd -> data.str,
                strlen(cmd -> data.str) + 1);
          }
      }
    else if (!strcmp(cmd -> name, "SearchType"))
      {
        if (!strcmp("regex", cmd -> data.str))
          { config -> searchtype = SEARCH_REGEX; }
        else if (!strcmp("regular", cmd -> data.str))
          { config -> searchtype = SEARCH_REGULAR; }
        else
          { return _("Illegal value for resource SearchType."); }
      }

    return NULL;
  }


/* #############################################################################
 *
 * Description    error handler for dotconf which I just use to get back wether
 *                an error occured; somehow dotconf_command_loop() does not
 *                return any useful error information
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      FUNC_ERRORHANDLER
 * Return         int
 */
FUNC_ERRORHANDLER(errorHandler)
  {
    fprintf(stderr, "%s:%ld: %s\n",
        configfile -> filename,
        configfile -> line,
        msg);

    configerror = 1;

    return 0;
  }


/* #############################################################################
 *
 * Description    handling of the resource file (default is ~/.cpmrc) using the
 *                dot-conf package
 * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      void
 * Return         1 for error, 0 if all is well
 */
int readResources(void)
  {
    configfile_t*       configfile;
    int                 error = 0;

    if (!config -> rcfile)
      { return 0; }

    /* open the rc file */
    configfile = dotconf_create(config -> rcfile,
        options,
        NULL,
        CASE_INSENSITIVE);
    if (!configfile)
      { error = 1; }
    else
      {
        configfile -> errorhandler = (dotconf_errorhandler_t)errorHandler;

        /* parse the configuration */
        if (dotconf_command_loop_until_error(configfile) ||
            configerror)
          {
            fprintf(stderr, _("error in configuration file.\n"));
            error = 1;
          }

        /* close the rc file */
        dotconf_cleanup(configfile);
      }

    return error;
  }


/* #############################################################################
 */

