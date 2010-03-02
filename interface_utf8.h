/* #############################################################################
 * header information for interface_utf8.c
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
#ifndef CPM_INTERFACE_UTF8_H
#define CPM_INTERFACE_UTF8_H


/* #############################################################################
 * includes
 */
#ifdef HAVE_LIBXML2
  #include <libxml/xmlversion.h>
  #include <libxml/encoding.h>
  #include <libxml/parser.h>
  #include <libxml/tree.h>
  #include <libxml/xpath.h>
#endif


/* #############################################################################
 * prototypes
 */

char* convert(int direction, char* instring);
char* convert2terminal(xmlChar* instring);
xmlChar* convert2xml(char* instring);
void freeUTF8Interface(void);
int initUTF8Encoding(char* encoding);


#endif


/* #############################################################################
 */

