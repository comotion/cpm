/* #############################################################################
 * header information for memory.c
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
#ifndef CPM_MEMORY_H
#define CPM_MEMORY_H

/* #############################################################################
 * prototypes
 */
#ifdef MEMDEBUG
  #define memAlloc(file, line, size) \
      memDebugAlloc(file, line, size)
  #define memFree(file, line, ptr, size) \
      memDebugFree(file, line, ptr, size)
  #define memFreeString(file, line, ptr) \
      memDebugFreeString(file, line, ptr)
  #define memRealloc(file, line, ptr, size_old, size_new) \
      memDebugRealloc(file, line, ptr, size_old, size_new)
#else
  #define memAlloc(file, line, size) \
      memRealAlloc(size);
  #define memFree(file, line, ptr, size) \
      memRealFree(ptr, size);
  #define memFreeString(file, line, ptr) \
      memRealFreeString(ptr);
  #define memRealloc(file, line, ptr, size_old, size_new) \
      memRealRealloc(ptr, size_old, size_new);
#endif

void* memDebugAlloc(const char* file, int line, size_t size);
void memDebugFree(const char* file, int line, void* ptr, size_t size);
void memDebugFreeString(const char* file, int line, void* ptr);
void* memDebugRealloc(const char* file, int line, void* ptr, int size_old,
    int size_new);

void* memRealAlloc(size_t size);
long int memCheck();
void memRealFree(void* ptr, size_t size);
void memRealFreeString(char* ptr);
void* memRealRealloc(void* ptr, size_t size_old, size_t size_new);

void* memSet(void* ptr, int value, size_t size);


#endif

/* #############################################################################
 */

