/* #############################################################################
 * interface to the xml library
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
#include <pwd.h>
#include <time.h>
#include "configuration.h"
#include "general.h"
#include "interface_utf8.h"
#include "interface_xml.h"
#include "listhandler.h"
#include "memory.h"
#include "patternparser.h"
#include "string.h"
#include "xml.h"


/* #############################################################################
 * internal functions
 */
void createEditorsNode(void);
void createTemplateNode(void);
int editorAdd(xmlChar* editor);
char* editorFindById(int uid);
int editorFindByName(char* editor);
static int nodeSort(const void* node1, const void* node2);
xmlNode* nodeFind(char* label);


/* #############################################################################
 * global variables
 */
xmlNode**               xmlwalklist;
xmlNode*                editorsnode;
xmlNode*                templatenode;
int                     level = 0,
                        maxeditor = 0,
                        maxlevel = 0;
/* Flawfinder: ignore */
char                    staticlabel[128];


/* #############################################################################
 *
 * Description    find the editors node or create a new one of it doesn't exist
 * Author         Harry Brueckner
 * Date           2005-04-06
 * Arguments      void
 * Return         void
 */
void createEditorsNode(void)
  {
    xmlNode*            curnode;
    xmlNode*            rootnode;
    char*               prop;

    TRACE(99, "createEditorsNode()", NULL);

    /* we walk along the root node to find the template node */
    rootnode = xmlGetDocumentRoot();
    curnode = rootnode -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "editor"))
          {
            editorsnode = curnode;
            break;
          }

        curnode = curnode -> next;
      }

    if (!editorsnode)
      {
        /* we add the default editors structure */
        editorsnode = xmlNewChild(rootnode,
            NULL,
            BAD_CAST "editor",
            NULL);
        if (!editorsnode)
          {
            fprintf(stderr,
                _("XML error: can not create XML structure (line %d)."),
                __LINE__);
            exit(1);
          }

        xmlSetCreation(editorsnode);
        editorAdd(convert2xml("unknown"));
      }

    /* we find the current maximum for uid */
    maxeditor = 0;
    curnode = editorsnode -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "user"))
          {
            prop = convert2terminal(xmlGetProp(curnode, BAD_CAST "uid"));
            if (prop)
              {   /* we found a user */
                /* Flawfinder: ignore */
                maxeditor = max(maxeditor, atoi(prop));
              }
          }

        curnode = curnode -> next;
      }
  }


/* #############################################################################
 *
 * Description    find the template node or create a new one of it doesn't exist
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      void
 * Return         void
 */
void createTemplateNode(void)
  {
    xmlNode*            curnode;
    xmlNode*            rootnode;

    TRACE(99, "createTemplateNode()", NULL);

    /* we walk along the root node to find the template node */
    rootnode = xmlGetDocumentRoot();
    curnode = rootnode -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "template"))
          {
            templatenode = curnode;
            break;
          }

        curnode = curnode -> next;
      }

    if (!templatenode)
      {
        /* we add the default template structure */
        templatenode = xmlNewChild(rootnode,
            NULL,
            BAD_CAST "template",
            NULL);
        if (!templatenode)
          {
            fprintf(stderr,
                _("XML error: can not create XML structure (line %d)."),
                __LINE__);
            exit(1);
          }

        xmlSetCreation(templatenode);
      }
  }


/* #############################################################################
 *
 * Description    add a new editor
 * Author         Harry Brueckner
 * Date           2005-04-06
 * Arguments      xmlChar* editor   - name of the editor
 *                                    NOTE: This must already be UTF-8!
 * Return         int user id of the editor
 */
int editorAdd(xmlChar* editor)
  {
    xmlNode*            curnode;
    int                 curuid;
    /* Flawfinder: ignore */
    char                uid[10];

    TRACE(99, "editorAdd()", NULL);

    if (!editorsnode)
      { createEditorsNode(); }

    curuid = editorFindByName((char*)editor);
    if (curuid != -1)
      { return curuid; }

    snprintf(uid, 10, "%d", ++maxeditor);

    curnode = xmlNewChild(editorsnode,
        NULL,
        BAD_CAST "user", editor);
    if (!curnode)
      {
        fprintf(stderr, _("XML error: can not create XML structure (line %d)."),
            __LINE__);
        exit(1);
      }
    xmlNewProp(curnode, BAD_CAST "uid", convert2xml(uid));
    xmlSetCreation(curnode);

    return maxeditor;
  }


/* #############################################################################
 *
 * Description    find an editor by his user id
 * Author         Harry Brueckner
 * Date           2005-04-06
 * Arguments      int uid   - user id of the editor
 * Return         char* name of the editor
 */
char* editorFindById(int uid)
  {
    xmlNode*            curnode;
    char*               prop;
    char*               result;
    char*               user;

    TRACE(99, "editorFindById()", NULL);

    if (!editorsnode)
      { createEditorsNode(); }

    if (uid < 1 || uid > maxeditor)
      { return NULL; }

    curnode = editorsnode -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "user"))
          {
            prop = convert2terminal(xmlGetProp(curnode, BAD_CAST "uid"));
            if (prop)
              {   /* we found a user */
                /* Flawfinder: ignore */
                if (uid == atoi(prop))
                  {
                    user = convert2terminal(xmlNodeGetContent(curnode));
                    result = memAlloc(__FILE__, __LINE__, strlen(user) + 1);
                    strStrncpy(result, user, strlen(user) + 1);

                    return result;
                  }
              }
          }

        curnode = curnode -> next;
      }

    return NULL;
  }


/* #############################################################################
 *
 * Description    find an editor by his name
 * Author         Harry Brueckner
 * Date           2005-04-06
 * Arguments      char* editor  - name of the editor
 * Return         int user id of the editor
 */
int editorFindByName(char* editor)
  {
    xmlNode*            curnode;
    int                 uid;
    char*               prop;
    char*               user;

    TRACE(99, "editorFindByName()", NULL);

    if (!editorsnode)
      { createEditorsNode(); }

    if (!editor)
      { return editorFindByName("unknown"); }

    curnode = editorsnode -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "user"))
          {
            user = convert2terminal(xmlNodeGetContent(curnode));
            if (!strcmp(user, editor))
              {   /* we found the user */
                prop = convert2terminal(xmlGetProp(curnode, BAD_CAST "uid"));
                if (prop)
                  {   /* we found a user */
                    /* Flawfinder: ignore */
                    uid = atoi(prop);
                    if (uid > 0)
                      { return uid; }
                    else
                      { return -1; }
                  }
              }
          }

        curnode = curnode -> next;
      }

    return -1;
  }


/* #############################################################################
 *
 * Description    free the XML parser stuff
 * Author         Harry Brueckner
 * Date           2005-03-18
 * Arguments      void
 * Return         void
 */
void freeXMLInterface(void)
  {
    TRACE(99, "freeXMLInterface()", NULL);

    if (xmlwalklist)
      {
        memFree(__FILE__, __LINE__, xmlwalklist, maxlevel * sizeof(xmlNode*));
        xmlwalklist = NULL;
      }
  }


/* #############################################################################
 *
 * Description    initialize the XML parser
 * Author         Harry Brueckner
 * Date           2005-03-18
 * Arguments      void
 * Return         void
 */
void initXMLInterface(void)
  {
    TRACE(99, "initXMLInterface()", NULL);

    level = 0;
    maxlevel = 0;

    editorsnode = NULL;
    templatenode = NULL;
    xmlwalklist = NULL;
  }


/* #############################################################################
 *
 * Description    see if a node witht the passed label exists in the current
 *                node
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      char* label - label of the node to find
 * Return         xmlNode* pointing to the found node
 */
xmlNode* nodeFind(char* label)
  {
    xmlNode*            curnode;
    xmlNode*            newnode;
    char*               prop;

    TRACE(99, "nodeFind()", NULL);

    if (!level || !label)
      { return 0; }

    curnode = xmlwalklist[level - 1] -> children;
    newnode = NULL;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "node"))
          {
            prop = convert2terminal(xmlGetProp(curnode, BAD_CAST "label"));
            if (prop &&
                !strcmp(label, prop))
              {   /* we found the label */
                return curnode;
              }
          }

        curnode = curnode -> next;
      }

    return NULL;
  }


/* #############################################################################
 *
 * Description    qsort function to sort the node names
 * Author         Harry Brueckner
 * Date           2005-03-27
 * Arguments      const void* node1   - label of node 1
 *                const void* node2   - label of node 2
 * Return         strcmp result of the names
 */
static int nodeSort(const void* node1, const void* node2)
  {
    char**              label1 = (char**)node1;
    char**              label2 = (char**)node2;

    TRACE(99, "nodeSort()", NULL);

    return strcmp(label1[0], label2[0]);
  }


/* #############################################################################
 *
 * Description    add a new node to the current one
 * Author         Harry Brueckner
 * Date           2005-03-24
 * Arguments      char* label - label of the new node
 * Return         void
 */
void xmlInterfaceAddNode(char* label)
  {
    xmlNode*            node;

    TRACE(99, "xmlInterfaceAddNode()", NULL);

    node = xmlNewChild(xmlwalklist[level - 1],
        NULL,
        BAD_CAST "node",
        BAD_CAST "");
    if (!node)
      {
        fprintf(stderr, _("XML error: can not create XML structure (line %d)."),
            __LINE__);
        exit(1);
      }

    xmlNewProp(node, BAD_CAST "label", convert2xml(label));
    xmlSetCreation(node);
  }


/* #############################################################################
 *
 * Description    delete a node from the current one
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      char* label - label of the node
 * Return         void
 */
void xmlInterfaceDeleteNode(char* label)
  {
    xmlNode*            node;

    TRACE(99, "xmlInterfaceDeleteNode()", NULL);

    node = nodeFind(label);
    if (!node)
      { return; }

    xmlUnlinkNode(node);
  }


/* #############################################################################
 *
 * Description    edit the label of a node
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      char* label_old   - old label of the node
 *                char* label_new   - new label of the node
 * Return         void
 */
void xmlInterfaceEditNode(char* label_old, char* label_new)
  {
    xmlNode*            node;

    TRACE(99, "xmlInterfaceEditNode()", NULL);

    node = nodeFind(label_old);
    if (!node)
      { return; }

    xmlSetProp(node, BAD_CAST "label", convert2xml(label_new));
    xmlSetModification(node);
  }


/* #############################################################################
 *
 * Description    free a list of node names
 * Author         Harry Brueckner
 * Date           2005-03-27
 * Arguments      char** array of node names
 * Return         void
 */
void xmlInterfaceFreeNames(char** list)
  {
    TRACE(99, "xmlInterfaceFreeNames()", NULL);

    listFree(list);
  }


/* #############################################################################
 *
 * Description    get the comment of a node
 * Author         Harry Brueckner
 * Date           2005-04-05
 * Arguments      char* label   - label of the node
 * Return         char* containing the data
 */
char* xmlInterfaceGetComment(char* label)
  {
    xmlNode*            commentnode = NULL;
    xmlNode*            curnode;
    xmlNode*            node;
    char*               comment;
    char*               result;

    TRACE(99, "xmlInterfaceGetComment()", NULL);

    node = nodeFind(label);
    if (!node)
      { return NULL; }

    curnode = node -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "comment"))
          {   /* we found our comment node, there can only be one */
            commentnode = curnode;
            break;
          }

        curnode = curnode -> next;
      }

    if (!commentnode)
      { return NULL; }

    comment = convert2terminal(xmlNodeGetContent(commentnode));
    result = memAlloc(__FILE__, __LINE__, strlen(comment) + 1);
    strStrncpy(result, comment, strlen(comment) + 1);

    return result;
  }


/* #############################################################################
 *
 * Description    get the creation label of a node
 * Author         Harry Brueckner
 * Date           2005-04-05
 * Arguments      char* label   - label of the node
 *                char** by     - the 'by' part of the label
 *                char** on     - the 'on' part of the label
 * Return         char* containing the data
 */
void xmlInterfaceGetCreationLabel(char* label, char** by, char**on)
  {
    xmlNode*            node;
    char*               propby;
    char*               propon;

    TRACE(99, "xmlInterfaceGetCreationLabel()", NULL);

    *by = NULL;
    *on = NULL;

    node = nodeFind(label);
    if (!node)
      { return; }

    propby = convert2terminal(xmlGetProp(node, BAD_CAST "created-by"));
    if (!propby || !strlen(propby))
      { propby = "0"; }
    /* Flawfinder: ignore */
    *by = editorFindById(atoi(propby));

    propon = convert2terminal(xmlGetProp(node, BAD_CAST "created-on"));
    if (!propon)
      { propon = "---"; }
    *on = memAlloc(__FILE__, __LINE__, strlen(propon) + 1);
    strStrncpy(*on, propon, strlen(propon) + 1);
  }


/* #############################################################################
 *
 * Description    get the modification label of a node
 * Author         Harry Brueckner
 * Date           2005-04-05
 * Arguments      char* label   - label of the node
 *                char** by     - the 'by' part of the label
 *                char** on     - the 'on' part of the label
 * Return         char* containing the data
 */
void xmlInterfaceGetModificationLabel(char* label, char** by, char**on)
  {
    xmlNode*            node;
    char*               propby;
    char*               propon;

    TRACE(99, "xmlInterfaceGetModificationLabel()", NULL);

    *by = NULL;
    *on = NULL;

    node = nodeFind(label);
    if (!node)
      { return; }

    propby = convert2terminal(xmlGetProp(node, BAD_CAST "modified-by"));
    if (!propby || !strlen(propby))
      { propby = "0"; }
    /* Flawfinder: ignore */
    *by = editorFindById(atoi(propby));

    propon = convert2terminal(xmlGetProp(node, BAD_CAST "modified-on"));
    if (!propon)
      { propon = "---"; }
    *on = memAlloc(__FILE__, __LINE__, strlen(propon) + 1);
    strStrncpy(*on, propon, strlen(propon) + 1);
  }


/* #############################################################################
 *
 * Description    get all node names of the current node
 * Author         Harry Brueckner
 * Date           2005-03-24
 * Arguments      void
 * Return         char** array of all nodes
 */
char** xmlInterfaceGetNames(void)
  {
    xmlNode*            curnode;
    xmlChar*            prop;
    char**              list;
    char*               pconv;
    int                 counter = 0,
                        i = 0;

    TRACE(99, "xmlInterfaceGetNames()", NULL);

    /* first we count how many children there are */
    curnode = xmlwalklist[level - 1] -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "node"))
          { counter++; }

        curnode = curnode -> next;
      }

    if (!counter)
      { return NULL; }

    /* then we allocate the memory */
    list = memAlloc(__FILE__, __LINE__, (counter + 1) * sizeof(char*));

    /* and finally we fill the array */
    curnode = xmlwalklist[level - 1] -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "node"))
          {
            prop = xmlGetProp(curnode, BAD_CAST "label");
            if (prop)
              {
                pconv = convert2terminal(prop);
                list[i] = memAlloc(__FILE__, __LINE__, strlen(pconv) + 1);
                strStrncpy(list[i], pconv, strlen(pconv) + 1);

                i++;
              }
          }

        curnode = curnode -> next;
      }

    list[i] = NULL;

    qsort(list, i, sizeof(char*), nodeSort);

    return list;
  }


/* #############################################################################
 *
 * Description    add another node to the walk list
 * Author         Harry Brueckner
 * Date           2005-03-24
 * Arguments      char* label - label of the node to find
 * Return         0 if all is ok, 1 if the node name could not be found
 */
int xmlInterfaceNodeDown(char* label)
  {
    xmlNode*            newnode;

    TRACE(99, "xmlInterfaceNodeDown()", NULL);

    if (!level)
      {   /* we start out with the root node */
        newnode = xmlGetDocumentRoot();
      }
    else
      {   /* we have to look for a node by its label */
        newnode = nodeFind(label);
      }

    if (!newnode)
      { return 1; }

    if (++level > maxlevel)
      {   /* reallocate as many pointers as needed at the moment */
        xmlwalklist = memRealloc(__FILE__, __LINE__, xmlwalklist,
            maxlevel * sizeof(xmlNode*),
            level * sizeof(xmlNode*));

        maxlevel = level;
      }

    xmlwalklist[level - 1] = newnode;

    return 0;
  }


/* #############################################################################
 *
 * Description    see if a node witht the passed label exists in the current
 *                node
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      char* label - label of the node to find
 * Return         xmlNode* pointing to the found node
 */
int xmlInterfaceNodeExists(char* label)
  {
    TRACE(99, "xmlInterfaceNodeExists()", NULL);

    if (nodeFind(label))
      { return 1; }
    else
      { return 0; }
  }


/* #############################################################################
 *
 * Description    get the label of a node if in the walklist
 * Author         Harry Brueckner
 * Date           2005-04-14
 * Arguments      int id  - id of the node to get
 * Return         char* label of the node
 */
char* xmlInterfaceNodeGet(int id)
  {
    TRACE(99, "xmlInterfaceNodeGet()", NULL);

    if (id < 1 ||
        id >= level)
      { return NULL; }

    return convert2terminal(xmlGetProp(xmlwalklist[id], BAD_CAST "label"));
  }


/* #############################################################################
 *
 * Description    remove one node from the walk list
 * Author         Harry Brueckner
 * Date           2005-03-24
 * Arguments      void
 * Return         void
 */
void xmlInterfaceNodeUp(void)
  {
    TRACE(99, "xmlInterfaceNodeUp()", NULL);

    if (level > 0 &&
        xmlwalklist[level - 1])
      { xmlwalklist[level - 1] = NULL; }

    if (level > 0)
      { level--; }
  }


/* #############################################################################
 *
 * Description    set a new comment
 * Author         Harry Brueckner
 * Date           2005-04-05
 * Arguments      char* label   - label of the node
 *                char* comment - the new comment
 * Return         void
 */
void xmlInterfaceSetComment(char* label, char* comment)
  {
    xmlNode*            commentnode = NULL;
    xmlNode*            curnode;
    xmlNode*            node;

    TRACE(99, "xmlInterfaceSetComment()", NULL);

    node = nodeFind(label);
    if (!node)
      { return; }

    curnode = node -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "comment"))
          {   /* we found our comment node, there can only be one */
            commentnode = curnode;
            break;
          }
        curnode = curnode -> next;
      }

    if (commentnode)
      {   /* if a comment exists, we update its modification */
        if (strlen(comment) == 0)
          {   /* if the comment is empty, we remove the node and are done */
            xmlUnlinkNode(commentnode);
            return;
          }

        xmlSetModification(commentnode);
      }
    else
      {   /* otherwise we have to create a new comment first */
        if (strlen(comment) == 0)
          {   /* if there is no comment, we must not do anything */
            return;
          }
        commentnode = xmlNewTextChild(node,
            NULL,
            BAD_CAST "comment",
            convert2xml(comment));
        if (!commentnode)
          {
            fprintf(stderr,
                _("XML error: can not create XML structure (line %d)."),
                __LINE__);
            exit(1);
          }
        xmlSetCreation(commentnode);
      }

    /* update the comment node */
    xmlNodeSetContent(commentnode,
        xmlEncodeCommentEntities(convert2xml(comment)));
    /* and its parent modification */
    xmlSetModification(node);
  }


/* #############################################################################
 *
 * Description    get the template title of the current node
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      int id          - the level of which we want the template of
 *                int is_static   - flag to indicate if the found template is
 *                                  statically defined (either by the XML
 *                                  structure or by the configuration)
 * Return         char* string with the title of the template
 */
char* xmlInterfaceTemplateGet(int id, int* is_static)
  {
    xmlNode*            curnode;
    char*               prop;

    TRACE(99, "xmlInterfaceTemplateGet()", NULL);

    *is_static = 1;

    if (!templatenode)
      { createTemplateNode(); }
    if (!templatenode)
      { return NULL; }

    if (id == -1)
      { id = level; }

    curnode = templatenode -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "title"))
          {
            prop = convert2terminal(xmlGetProp(curnode, BAD_CAST "level"));
            if (prop &&
                /* Flawfinder: ignore */
                atoi(prop) == id)
              {   /* we found the title for the id */
                return convert2terminal(xmlNodeGetContent(curnode));
              }
          }

        curnode = curnode -> next;
      }

    if (config -> defaulttemplates &&
        config -> defaulttemplates[id - 1])
      {   /* we have a default template name */
        return config -> defaulttemplates[id - 1];
      }

    *is_static = 0;

    /* no default defined, so we use a dynamic default */
    snprintf(staticlabel, 128, _("level %d"), id);
    return staticlabel;
  }


/* #############################################################################
 *
 * Description    get the template id of the given template title
 * Author         Harry Brueckner
 * Date           2005-04-12
 * Arguments      char* title   - title of the template
 * Return         template id or -1 if the title is not found
 */
int xmlInterfaceTemplateGetId(char* title)
  {
    xmlNode*            curnode;
    int                 tid;
    char*               prop;

    TRACE(99, "xmlInterfaceTemplateGetId()", NULL);

    if (!templatenode)
      { createTemplateNode(); }
    if (!templatenode)
      { return -1; }

    curnode = templatenode -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "title") &&
            !strcmp(convert2terminal(xmlNodeGetContent(curnode)), title))
          {   /* we found the title */
            prop = convert2terminal(xmlGetProp(curnode, BAD_CAST "level"));
            if (prop)
              {
                /* Flawfinder: ignore */
                tid = atoi(prop);
                if (tid > 0)
                  { return tid; }
                else
                  { return -1; }
              }
          }

        curnode = curnode -> next;
      }

    return -1;
  }


/* #############################################################################
 *
 * Description    set the template title of the current node
 * Author         Harry Brueckner
 * Date           2005-03-24
 * Arguments      char* title   - new title to set
 * Return         void
 */
void xmlInterfaceTemplateSet(char* title)
  {
    xmlNode*            curnode;
    char*               prop;
    /* Flawfinder: ignore */
    char                levelstr[32];

    TRACE(99, "xmlInterfaceTemplateSet()", NULL);

    if (!templatenode)
      { createTemplateNode(); }
    if (!templatenode)
      { return; }

    curnode = templatenode -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "title"))
          {
            prop = convert2terminal(xmlGetProp(curnode, BAD_CAST "level"));
            if (prop &&
                /* Flawfinder: ignore */
                atoi(prop) == level)
              {   /* we found the title for the current level */
                xmlNodeSetContent(curnode, convert2xml(title));
                xmlSetModification(curnode);
                return;
              }
          }

        curnode = curnode -> next;
      }

    /* the title is not yet set, so we create a new one */
    curnode = xmlNewChild(templatenode,
        NULL,
        BAD_CAST "title",
        convert2xml(title));
    if (!curnode)
      {
        fprintf(stderr, _("XML error: can not create XML structure (line %d)."),
            __LINE__);
        exit(1);
      }
    snprintf(levelstr, 32, "%d", level);
    xmlNewProp(curnode, BAD_CAST "level", convert2xml(levelstr));
    xmlSetCreation(curnode);
  }


/* #############################################################################
 *
 * Description    walk through the whole tree and hand all label combinations to
 *                the CLI callback function
 * Author         Harry Brueckner
 * Date           2005-04-06
 * Arguments      xmlNode* node   - the node to walk through; if NULL is passed
 *                                  the document root is used
 *                char*** path    - path we are currently recursing through
 *                WALKFN walker   - callback function where the matching can be
 *                                  processed
 * Return         int number of found patterns
 */
int xmlInterfaceTreeWalk(xmlNode* node, char*** path, WALKFN walker)
  {
    xmlNode*            curnode;
    char*               label;
    int                 found = 0,
                        level = listCount(*path);

    TRACE(99, "xmlInterfaceTreeWalk()", NULL);

    if (!node)
      {   /* if we did not get a node, we start at the root element */
        node = xmlGetDocumentRoot();
      }

    curnode = node -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE &&
            !strcmp((char*)curnode -> name, "node"))
          {
            label = convert2terminal(xmlGetProp(curnode, BAD_CAST "label"));
            if (label)
              {   /* we found a node we have to use in the search */
                *path = listAdd(*path, label);

                /* we call the walker function for further processing */
                found += (walker)(*path);

                /* and we recurse further down */
                found += xmlInterfaceTreeWalk(curnode, path, walker);

                *path = listDelete(*path, level);
              }
          }

        curnode = curnode -> next;
      }

    return found;
  }


/* #############################################################################
 *
 * Description    add the current realm and the timestamp to a given node
 * Author         Harry Brueckner
 * Date           2005-04-04
 * Arguments      xmlChar* uidlabel   - label for the realm name
 *                xmlChar* timelabel  - label for the timestamp
 *                xmlNode* node       - the node to walk
 * Return         void
 */
void xmlInterfaceUpdateTimestamp(xmlChar* uidlabel, xmlChar* timelabel,
    xmlNode* curnode)
  {
    time_t              now;
    struct tm*          tdata;
    xmlChar*            prop;
    /* Flawfinder: ignore */
    char                tdisplay[21];
    /* Flawfinder: ignore */
    char                udisplay[10];

    TRACE(99, "xmlInterfaceUpdateTimestamp()", NULL);

    if (!curnode)
      { return; }

    /* these names can not have a creator */
    if (!strcmp("editor", (char*)curnode -> name) ||
        !strcmp("template", (char*)curnode -> name) ||
        !strcmp("title", (char*)curnode -> name) ||
        !strcmp("user", (char*)curnode -> name))
      { uidlabel = NULL; }

    if (uidlabel)
      {
        /* we create the new user */
        snprintf(udisplay, 10, "%d", editorAdd((xmlChar*)runtime -> realm));

        /* we update the uid label */
        prop = xmlGetProp(curnode, uidlabel);
        if (prop)
          { xmlSetProp(curnode, uidlabel, convert2xml(udisplay)); }
        else
          { xmlNewProp(curnode, uidlabel, convert2xml(udisplay)); }
      }

    if (timelabel)
      {
        now = time(NULL);
        tdata = localtime(&now);
        snprintf(tdisplay, 21, "%04d-%02d-%02d %02d:%02d:%02d",
            tdata -> tm_year + 1900,
            tdata -> tm_mon + 1,
            tdata -> tm_mday,
            tdata -> tm_hour,
            tdata -> tm_min,
            tdata -> tm_sec);

        /* we update the time label */
        prop = xmlGetProp(curnode, timelabel);
        if (prop)
          { xmlSetProp(curnode, timelabel, convert2xml(tdisplay)); }
        else
          { xmlNewProp(curnode, timelabel, convert2xml(tdisplay)); }
      }
  }


/* #############################################################################
 */

