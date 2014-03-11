/* #############################################################################
 * header information for options.c
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
#ifndef CPM_OPTIONS_H
#define CPM_OPTIONS_H


/* #############################################################################
 * includes
 */
/* we need this for the version info only */
#include <cdk.h>


/* #############################################################################
 * prototypes
 */
void getDefaultOptions(void);
char* getFilePath(char* path, char* filename);
int getOptions(int argc, char **argv);
void showHelp(void);
void showVersion(void);

#endif

/* #############################################################################
 */

