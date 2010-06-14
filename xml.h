/* #############################################################################
 * header information for xml.c
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
#ifndef CPM_XML_H
#define CPM_XML_H


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
#include "gpg.h"


/* #############################################################################
 * prototypes
 */
void freeXML(void);
void initXML(void);
int xmlDataFileRead(char* filename, char** errormsg,
    PASSPHRASE_FN passphrase_cb, SHOWERROR_FN showerror_cb);
int xmlDataFileWrite(char* filename, char** errormsg,
    PASSPHRASE_FN passphrase_cb, SHOWERROR_FN showerror_cb);
xmlChar* xmlEncodeCommentEntities(xmlChar* string);
xmlNode* xmlGetDocumentRoot(void);


#endif


/* #############################################################################
 */

