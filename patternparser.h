/* #############################################################################
 * header information for patternparser.c
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
#ifndef CPM_PATTERNPARSER_H
#define CPM_PATTERNPARSER_H


/* #############################################################################
 * global interface variables
 */
typedef struct sSearchPattern SEARCHPATTERN;
struct sSearchPattern
  {
    int                 type;
    int                 templateid;
    char*               string;
    SEARCHPATTERN*      next;
  };

#define PATTERN_UNDEF     0
#define PATTERN_STRING    1
#define PATTERN_TEMPLATE  2


/* #############################################################################
 * prototypes
 */
void freePatternparser(void);
int getPatternResultString(int id, char** path, char** string);
int getPatternSearchString(int id, char** path, char** string);
void initPatternparser(void);
int patternParse(void);


#endif


/* #############################################################################
 */

