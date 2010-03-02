/* #############################################################################
 * header information for interface_xml.c
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
#ifndef CPM_INTERFACE_XML_H
#define CPM_INTERFACE_XML_H


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
 * global variables
 */
typedef int (*WALKFN) (char** path);


/* #############################################################################
 * prototypes
 */
void createEditorsNode(void);
void createTemplateNode(void);
void freeXMLInterface(void);
int initXMLEncoding(char* encoding);
void initXMLInterface(void);
void xmlInterfaceAddNode(char* label);
void xmlInterfaceDeleteNode(char* label);
void xmlInterfaceEditNode(char* label_old, char* label_new);
void xmlInterfaceFreeNames(char** list);
char* xmlInterfaceGetComment(char* label);
void xmlInterfaceGetCreationLabel(char* label, char** by, char**on);
void xmlInterfaceGetModificationLabel(char* label, char** by, char**on);
char** xmlInterfaceGetNames(void);
int xmlInterfaceNodeDown(char* label);
int xmlInterfaceNodeExists(char* label);
char* xmlInterfaceNodeGet(int id);
void xmlInterfaceNodeUp(void);
void xmlInterfaceSetComment(char* label, char* comment);
char* xmlInterfaceTemplateGet(int id, int* is_static);
int xmlInterfaceTemplateGetId(char* title);
void xmlInterfaceTemplateSet(char* title);
int xmlInterfaceTreeWalk(xmlNode* node, char*** path, WALKFN walker);
void xmlInterfaceUpdateTimestamp(xmlChar* uidlabel, xmlChar* timelabel,
    xmlNode* curnode);


#define xmlSetModification(node)  xmlInterfaceUpdateTimestamp( \
                                    BAD_CAST "modified-by", \
                                    BAD_CAST "modified-on", \
                                    node)
#define xmlSetCreation(node)      xmlInterfaceUpdateTimestamp( \
                                    BAD_CAST "created-by", \
                                    BAD_CAST "created-on", \
                                    node)

#endif


/* #############################################################################
 */

