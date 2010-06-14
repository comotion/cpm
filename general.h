/* #############################################################################
 * header information for general.c
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
#ifndef CPM_GENERAL_H
#define CPM_GENERAL_H

/* #############################################################################
 * prototypes
 */
#ifdef TRACE_DEBUG
void cpm_trace(const char* filename, int line, int level, char* fmt, ...);
#endif
int createBackupfile(char* filename, SHOWERROR_FN showerror_cb);
char* createPassword(int length);
int fileExists(char* filename);
int fileLockCreate(char* filename, char* extension, char** errormsg);
int fileLockOpen(char* filename, int flags, mode_t mode, char** errormsg);
int fileLockRemove(char** errormsg);
char* isGoodPassword(char* password);
int isReadonly(char* filename);
char* resolveFilelink(char* filename);
#ifndef HAVE_STRERROR
  char *strerror(int errnum);
#endif


/* #############################################################################
 * Macro to call the trace routine
 */

#ifdef TRACE_DEBUG
#define TRACE(level, fmt, args...) cpm_trace(__FILE__, __LINE__, level, fmt, ##args)
#else
#define TRACE(level, fmt, args...) ;
#endif


#endif

/* #############################################################################
 */

