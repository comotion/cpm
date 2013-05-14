/* #############################################################################
 * code for memory allocation handling
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


/* #############################################################################
 * global variables
 */
long int                memorycounter = 0;


/* #############################################################################
 *
 * Description    allocate memory of the given size
 * Author         Harry Brueckner
 * Date           2005-04-11
 * Arguments      see memRealAlloc
 * Return         see memRealAlloc
 */
void* memDebugAlloc(const char* file, int line, size_t size)
  {
    fprintf(stderr, "alloc  %5lu (%s, line %d)\n", size, file, line);
    return memRealAlloc(size);
  }


/* #############################################################################
 *
 * Description    free the given memory block
 * Author         Harry Brueckner
 * Date           2005-04-11
 * Arguments      see memRealFree
 * Return         see memRealFree
 */
void memDebugFree(const char* file, int line, void* ptr, size_t size)
  {
    fprintf(stderr, "free   %5lu (%s, line %d)\n", size, file, line);
    memRealFree(ptr, size);
  }


/* #############################################################################
 *
 * Description    free the given memory block
 * Author         Harry Brueckner
 * Date           2005-04-11
 * Arguments      see memRealFree
 * Return         see memRealFree
 */
void memDebugFreeString(const char* file, int line, void* ptr)
  {
    if (ptr)
      {
        fprintf(stderr, "free   %5zd (%s, line %d)\n",
            strlen(ptr) + 1, file, line);
      }

    memRealFreeString(ptr);
  }


/* #############################################################################
 *
 * Description    reallocate memory of the given size
 * Author         Harry Brueckner
 * Date           2005-04-11
 * Arguments      see memRealRealloc
 * Return         see memRealRealloc
 */
void* memDebugRealloc(const char* file, int line, void* ptr, int size_old,
    int size_new)
  {
    if (size_old)
      { fprintf(stderr, "rfree  %5d (%s, line %d)\n", size_old, file, line); }
    fprintf(stderr, "ralloc %5d (%s, line %d)\n", size_new, file, line);

    return memRealRealloc(ptr, size_old, size_new);
  }


/* #############################################################################
 *
 * Description    allocate memory of the given size
 * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      size_t size - size of the area to allocate
 * Return         pointer to the allocated memory
 */
void* memRealAlloc(size_t size)
  {
    void*               ptr;

    ptr = malloc(size);
    if (!ptr)
      {
        fprintf(stderr, _("out of memory error - tried to allocate %lu byte.\n"),
            size);
        exit(1);
      }

    /* update the memory counter */
    memorycounter += size;

    return ptr;
  }


/* #############################################################################
 *
 * Description    check if the memory is free like it was when we started
 * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      void
 * Return         0 if ok, 1 if more or less memory is used
 */
long int memCheck(void)
  {
    return memorycounter;
  }


/* #############################################################################
 *
 * Description    free the given memory block
 * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      char* ptr   - pointer to the given area
 *                size_t size - size of the area to free
 * Return         void
 */
void memRealFree(void* ptr, size_t size)
  {
    if (!ptr)
      { return; }

    /* update the memory counter */
    memorycounter -= size;

    memSet(ptr, 0, size);

    free(ptr);
  }


/* #############################################################################
 *
 * Description    free the given string                                          * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      void* ptr   - pointer to the given area
 * Return         void
 */
void memRealFreeString(char* ptr)
  {
    if (!ptr)
        return;

    memRealFree(ptr, strlen(ptr) + 1);
  }



/* #############################################################################
 *
 * Description    reallocate memory of the given size
 * Author         Harry Brueckner
 * Date           2005-03-18
 * Arguments      void* ptr
 *                size_t size_old - old size of the area
 *                size_t size_new - new size of the area
 * Return         pointer to the allocated memory
 */
void* memRealRealloc(void* ptr, size_t size_old, size_t size_new)
  {
    ptr = realloc(ptr, size_new);
    if (!ptr)
      {
        fprintf(stderr,
            _("out of memory error - tried to reallocate %lu byte.\n"),
            size_new);
        exit(1);
      }

    /* update the memory counter */
    memorycounter -= size_old;
    memorycounter += size_new;

    return ptr;
  }


/* #############################################################################
 *
 * Description    memset which should not get optimized by any compiler; using
 *                this function, a 'dead store removal' problem should not
 *                occur;
 *                this code was taken from 'Secure Programming for Linux and
 *                Unix HOWTO' p. 129 ff.
 * Author         Harry Brueckner
 * Date           2005-05-17
 * Arguments      void* ptr     - pointer to the array
 *                int value     - value to set
 *                size_t size   - size of the array
 * Return         pointer to the allocated memory
 */
void* memSet(void* ptr, int value, size_t size)
  {
    volatile char*      p = ptr;

    while (size--)
      { *p++ = value; }

    return ptr;
  }


/* #############################################################################
 */

