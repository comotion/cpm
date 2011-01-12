/* #############################################################################
 * header information for security.c
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
#ifndef CPM_SECURITY_H
#define CPM_SECURITY_H

/* #############################################################################
 * prototypes
 */
int checkSecurity(int silent);
int initSecurity(int* max_mem_lock, int* memory_safe, int* ptrace_safe, rlim_t* memlock_limit);
void listEnvironment(void);
void testEnvironment(void);

#ifndef MEMLOCK_LIMIT
  #define MEMLOCK_LIMIT 5120
#endif

#ifdef __sun__
  /* Solaris does not have the max. memory lock check */
  #ifndef NO_MEMLOCK
    #define MAX_SECURITY_LEVEL  6
  #else
    #define MAX_SECURITY_LEVEL  4
  #endif
#else
  #ifndef NO_MEMLOCK
    #define MAX_SECURITY_LEVEL  7
  #else
    #define MAX_SECURITY_LEVEL  5
  #endif
#endif


#endif

/* #############################################################################
 */

