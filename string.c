/* #############################################################################
 * code for string handling
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
#include "memory.h"
#include "string.h"


/* #############################################################################
 *
 * Description    create a new string from the two given ones
 * Author         Harry Brueckner
 * Date           2005-05-30
 * Arguments      char* str1  - part 1 of the string
 *                char* str1  - part 2 of the string
 * Return         char* pointing to the new string
 */
char* strNewcat(char* str1, char* str2)
  {
    int                 size;
    char*               buffer;

    if (str2)
      { size = strlen(str1) + strlen(str2) + 1; }
    else
      { size = strlen(str1) + 1; }
    buffer = memAlloc(__FILE__, __LINE__, size);
    /* Flawfinder: ignore */
    strcpy(buffer, str1);
    if (str2)
      {
        /* Flawfinder: ignore */
        buffer = strcat(buffer, str2);
      }

    return buffer;
  }


/* #############################################################################
 *
 * Description    same as strncat but it's always \0 terminated
 * Author         Harry Brueckner
 * Date           2005-05-17
 * Arguments      see strncat
 * Return         see strncat
 */
char* strStrncat(char* dest, const char* src, size_t n)
  {
    int                 size;
    char*               buffer;

    size = strlen(dest);
    buffer = strncat(dest, src, n);
    buffer[size + n - 1] = 0;

    return buffer;
  }


/* #############################################################################
 *
 * Description    same as strncpy but it's always \0 terminated
 * Author         Harry Brueckner
 * Date           2005-05-17
 * Arguments      see strncpy
 * Return         see strncpy
 */
char* strStrncpy(char* dest, const char* src, size_t n)
  {
    char*               buffer;

    buffer = strncpy(dest, src, n - 1);
    buffer[n - 1] = 0;

    return buffer;
  }


/* #############################################################################
 */

