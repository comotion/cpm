/* #############################################################################
 * general list handler which handles any NULL terminated lists
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
#include "general.h"
#include "memory.h"
#include "listhandler.h"
#include "string.h"


/* #############################################################################
 * internal functions
 */
static int listSortKey(const void* element1, const void* element2);


/* #############################################################################
 *
 * Description    add the given element to the list
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      char** list   - list to modify
 *                char* element - new element to add
 * Return         void
 */
char** listAdd(char** list, char*  element)
  {
    int                 entries;

    entries = listCount(list);
    if (entries)
      {
        list = memRealloc(__FILE__, __LINE__, list,
            sizeof(char*) * (entries + 1),
            sizeof(char*) * (entries + 2));
      }
    else
      {
        list = memAlloc(__FILE__, __LINE__, sizeof(char*) * 2);
      }

    list[entries] = memAlloc(__FILE__, __LINE__, strlen(element) + 1);
    strStrncpy(list[entries], element, strlen(element) + 1);
    list[entries + 1] = NULL;

    return list;
  }


/* #############################################################################
 *
 * Description    count the entries in the list
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      char** list   - list to count
 * Return         number of elements in the list
 */
int listCount(char** list)
  {
    int                 id = 0;

    if (list)
      {
        while (list[id])
          { id++; }
      }

    return id;
  }


/* #############################################################################
 *
 * Description    delete the given element id from the list
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      char** list   - list to modify
 *                int id        - id of the element
 * Return         void
 */
char** listDelete(char** list, int id)
  {
    int                 entries,
                        i;

    entries = listCount(list);
    if (id < 0 ||
        id >= entries)
      { return list; }

    /* we first free the useless string */
    memFreeString(__FILE__, __LINE__, list[id]);

    /* we move all elements one id towards the beginning of the array;
     * since we know that we have 'entries + 1' elements allocated
     */
    for (i = id; i < entries; i++)
        list[i] = list[i + 1];

    if (entries == 1)
      {   /* this was our last element, we free the whole array */
        memFree(__FILE__, __LINE__, list, sizeof(char*) * 2);
        list = NULL;
      }
    else
      {   /* we reduce the size of the array */
        list = memRealloc(__FILE__, __LINE__, list,
            sizeof(char*) * (entries + 1),
            sizeof(char*) * (entries));
      }

    return list;
  }


/* #############################################################################
 *
 * Description    free the whole list structure
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      char** list   - list to free
 * Return         void
 */
char** listFree(char** list)
  {
    int                 id = 0;;

    if (list)
      {
        while (list[id])
          {
            memFreeString(__FILE__, __LINE__, list[id]);
            id++;
          }

        memFree(__FILE__, __LINE__, list, sizeof(char*) * (id + 1));
        list = NULL;
      }

    return list;
  }


/* #############################################################################
 *
 * Description    sort the list
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      char** list   - list to sort
 * Return         void
 */
void listSort(char** list)
  {
    int                 entries;

    entries = listCount(list);
    qsort(list, entries, sizeof(char*), listSortKey);
  }


/* #############################################################################
 *
 * Description    qsort function to sort the list entries
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      const void* element1  - label of element 1
 *                const void* element2  - label of element 2
 * Return         strcmp result of the names
 */
static int listSortKey(const void* element1, const void* element2)
  {
    char**              label1 = (char**)element1;
    char**              label2 = (char**)element2;

    return strcmp(label1[0], label2[0]);
  }


/* #############################################################################
 */

