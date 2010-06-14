/* #############################################################################
 * code for all things regarding the key list.
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
#include "interface_keys.h"
#include "interface_utf8.h"
#include "listhandler.h"
#include "memory.h"


/* #############################################################################
 * global variables
 */
char**                  encrypttionkeylist;


/* #############################################################################
 *
 * Description    free the keys
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      void
 * Return         void
 */
void freeKeys(void)
  {
    TRACE(99, "freeKeys()", NULL);

    encrypttionkeylist = listFree(encrypttionkeylist);
  }


/* #############################################################################
 *
 * Description    initialize the key handling
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      void
 * Return         void
 */
void initKeys(void)
  {
    TRACE(99, "initKeys()", NULL);

    encrypttionkeylist = NULL;
  }


/* #############################################################################
 *
 * Description    add a key
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      char* key   - the key to add
 * Return         int 1 if the key was added, otherwise 0
 */
int keyAdd(char* key)
  {
    char*               identifier;
    char*               tname;

    TRACE(99, "keyAdd()", NULL);

    if (!key || !strlen(key))
      { return 0; }

    tname = (char*)convert2xml(key);
    identifier = gpgValidateEncryptionKey(tname);

    if (identifier)
      {
        if (keyGetId(identifier) == -1)
          {   /* we don't know this key yet */
            encrypttionkeylist = listAdd(encrypttionkeylist, identifier);
            listSort(encrypttionkeylist);
          }

        /* we must free the identifier string */
        memFreeString(__FILE__, __LINE__, identifier);

        return 1;
      }
    else
      { return 0; }
  }


/* #############################################################################
 *
 * Description    modify a key
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      int id      - id of the key to modify
 *                char* key   - new key value
 * Return         int 1 if the key was added, otherwise 0
 */
int keyChange(int id, char* key)
  {
    int                 entries;
    char*               identifier;
    char*               tname;

    TRACE(99, "keyChange()", NULL);

    if (!key || !strlen(key))
      { return 0; }

    entries = listCount(encrypttionkeylist);

    if (id < 0 ||
        id >= entries)
      { return 0; }

    tname = (char*)convert2xml(key);
    identifier = gpgValidateEncryptionKey(tname);
    if (identifier)
      {
        if (keyGetId(identifier) == -1)
          {   /* the key already exists, so we ignore it */
            memFreeString(__FILE__, __LINE__, identifier);
          }
        else
          {   /* we don't know this key */
            memFreeString(__FILE__, __LINE__, encrypttionkeylist[id]);
            encrypttionkeylist[id] = identifier;
            listSort(encrypttionkeylist);
          }

        return 1;
      }
    else
      { return 0; }
  }


/* #############################################################################
 *
 * Description    count how many keys we have
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      void
 * Return         int number of currently available keys
 */
int keyCount(void)
  {
    TRACE(99, "keyCount()", NULL);

    return listCount(encrypttionkeylist);
  }


/* #############################################################################
 *
 * Description    set the used keys to the defaults found in the resource
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      void
 * Return         void
 */
void keyDefaults(void)
  {
    int                 i;

    TRACE(99, "keyDefaults()", NULL);

    for (i = listCount(config -> defaultkeys); i > 0; i--)
      {
        if (!keyAdd(config -> defaultkeys[i - 1]))
          {
            fprintf(stderr,
                _("error: encryption key %s could not be validated; not using it.\n"),
                config -> defaultkeys[i - 1]);
          }
      }
  }


/* #############################################################################
 *
 * Description    delete a key
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      int id  - id of the key to delete
 * Return         void
 */
void keyDelete(int id)
  {
    TRACE(99, "keyDelete()", NULL);

    encrypttionkeylist = listDelete(encrypttionkeylist, id);
  }


/* #############################################################################
 *
 * Description    get a key by its index
 * Author         Harry Brueckner
 * Date           2005-04-03
 * Arguments      int id  - id of the key to get
 * Return         char* pointing to the key
 */
char* keyGet(int id)
  {
    TRACE(99, "keyGet()", NULL);

    return encrypttionkeylist[id];
  }


/* #############################################################################
 *
 * Description    get a key id by its value
 * Author         Harry Brueckner
 * Date           2005-04-13
 * Arguments      char* key   - key name
 * Return         int id of the key, if its not found -1 is returned
 */
int keyGetId(char* key)
  {
    int                 i;

    TRACE(99, "keyGetId()", NULL);

    for (i = listCount(encrypttionkeylist); i > 0; i--)
      {
        if (!strcmp(encrypttionkeylist[i - 1], key))
            return i - 1;
      }

    return -1;
  }


/* #############################################################################
 *
 * Description    get the key list
 * Author         Harry Brueckner
 * Date           2005-04-03
 * Arguments      void
 * Return         char** list of all keys
 */
char** keyGetList(int id)
  {
    TRACE(99, "keyGetList()", NULL);

    return encrypttionkeylist;
  }


/* #############################################################################
 */

