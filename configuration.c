/* #############################################################################
 * code for configuration handling
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
#ifdef HAVE_LIBZ
  #include <zlib.h>
#endif
#include "configuration.h"
#include "general.h"
#include "listhandler.h"
#include "memory.h"

cpmconfig_t*            config;
cpmruntime_t*           runtime;

/* #############################################################################
 *
 * Description    clear the passphrase memory
 * Author         Harry Brueckner
 * Date           2005-04-03
 * Arguments      int final   - if set to 1, the passphrase is cleared under
 *                              all circumstances; otherwise only if the
 *                              keeppassphrase option is not set
 * Return         void
 */
void clearPassphrase(int final)
  {
    TRACE(99, "clearPassphrase()", NULL);

    if (final || !config -> keeppassphrase)
      {
        memSet(runtime -> passphrase, 0, PASSPHRASE_LENGTH);
      }
  }


/* #############################################################################
 *
 * Description    free the configuration
 * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      void
 * Return         void
 */
void freeConfiguration(void)
  {
    TRACE(99, "freeConfiguration()", NULL);

    if (config)
      {
        config -> defaultkeys = listFree(config -> defaultkeys);
        config -> defaulttemplates = listFree(config -> defaulttemplates);
        config -> defaulttemplatestatus =
            listFree(config -> defaulttemplatestatus);
        config -> searchdata = listFree(config -> searchdata);

#ifdef TEST_OPTION
        memFreeString(__FILE__, __LINE__, config -> testrun);
#endif

        if (config -> dbfilerc)
          { memFreeString(__FILE__, __LINE__, config -> dbfilerc); }
        if (config -> dbfilecmd)
          { memFreeString(__FILE__, __LINE__, config -> dbfilecmd); }
        if (config -> rcfile)
          { memFreeString(__FILE__, __LINE__, config -> rcfile); }
        if (config -> encoding)
          { memFreeString(__FILE__, __LINE__, config -> encoding); }
        if (config -> passwordalphabet)
          { memFreeString(__FILE__, __LINE__, config -> passwordalphabet); }

        memFree(__FILE__, __LINE__, config, sizeof(cpmconfig_t));
        config = NULL;
      }

    if (runtime)
      {
        runtime -> resultpatterns = listFree(runtime -> resultpatterns);
        runtime -> searchpatterns = listFree(runtime -> searchpatterns);

        clearPassphrase(1);
        memFreeString(__FILE__, __LINE__, runtime -> realm);
        memFreeString(__FILE__, __LINE__, runtime -> realmhint);

        if (runtime -> dbfile)
          { memFreeString(__FILE__, __LINE__, runtime -> dbfile); }
        if (runtime -> lockfile)
          { memFreeString(__FILE__, __LINE__, runtime -> lockfile); }

        memFree(__FILE__, __LINE__, runtime, sizeof(cpmruntime_t));
        config = NULL;
      }
  }


/* #############################################################################
 *
 * Description    initialize the global configuration structure
 * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      void
 * Return         void
 */
void initConfiguration(void)
  {
    TRACE(99, "initConfiguration()", NULL);

    config = memAlloc(__FILE__, __LINE__, sizeof(cpmconfig_t));
    runtime = memAlloc(__FILE__, __LINE__, sizeof(cpmruntime_t));

    config -> defaultkeys = NULL;
    config -> defaulttemplates = NULL;
    config -> defaulttemplatestatus = NULL;
    config -> searchdata = NULL;
    config -> dbfilerc = NULL;
    config -> dbfilecmd = NULL;
    config -> rcfile = NULL;
    config -> encoding = NULL;
    config -> passwordalphabet = NULL;

    clearPassphrase(1);
#ifdef TEST_OPTION
    config -> testrun = NULL;
#endif
    config -> hidecharacter = '*';

    config -> asktoquit = 0;
    config -> casesensitive = 1;
    config -> compression = Z_BEST_COMPRESSION;
    config -> configtest = 0;
    config -> cracklibstatus = CRACKLIB_ON;
    config -> createbackup = 1;
    config -> debuglevel = 0;
    config -> encryptdata = 1;
    config -> environtmentlist = 0;
    config -> help = 0;
    config -> infoheight = 5;
    config -> keeppassphrase = 0;
    config -> passwordlength = 10;
    config -> readonly = 0;
    config -> searchtype = SEARCH_REGULAR;
    config -> security = 0;
    config -> templatelock = 0;
    config -> version = 0;
    config -> inactivetimeout = 0;

    runtime -> memlock_limit = -2;

    runtime -> resultpatterns = NULL;
    runtime -> searchpatterns = NULL;
    runtime -> dbfile = NULL;
    runtime -> lockfile = NULL;
    runtime -> realm = NULL;
    runtime -> realmhint = NULL;

    runtime -> casesensitive = -1;
    runtime -> commandlinekeys = 0;
    runtime -> datachanged = 0;
    runtime -> guimode = 0;
    runtime -> lockfilecreated = 0;
    runtime -> max_mem_lock = 0;
    runtime -> memory_safe = 0;
    runtime -> ptrace_safe = 0;
    runtime -> readonly = 0;
    runtime -> searchtype = SEARCH_UNDEF;
    runtime -> updatestatus = 0;
  }


/* #############################################################################
*/

