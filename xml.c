/* #############################################################################
 * code for all xml stuff
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
#include <stdarg.h>
#ifdef HAVE_LIBZ
  #include <zlib.h>
#endif
#include "configuration.h"
#include "general.h"
#include "gpg.h"
#include "interface_keys.h"
#include "interface_xml.h"
#include "listhandler.h"
#include "memory.h"
#include "string.h"
#include "xml.h"
#include "zlib.h"


/* #############################################################################
 * internal functions
 */
int checkDtd(SHOWERROR_FN showerror_cb);
void xmlRemoveDtd(void);
void xmlVersionNodeUpdate(long oldversion, xmlNode* rootnode);
void xmlVersionUpdate(int silent);
void xmlValidateError(void* context, const char* msg, ...);
void xmlValidateWarning(void* context, const char* msg, ...);


/* #############################################################################
 * global variables
 */
xmlDocPtr               xmldoc;
SHOWERROR_FN            validateShowError = NULL;
const static char*      dtd_1 =
    "<!ENTITY % creation \"\n"
    "    created-by    CDATA #REQUIRED\n"
    "    created-on    CDATA #REQUIRED\n"
    "    \">\n"
    "<!ENTITY % creationtime \"\n"
    "    created-on    CDATA #REQUIRED\n"
    "    \">\n"
    "<!ENTITY % modification \"\n"
    "    modified-by   CDATA #IMPLIED\n"
    "    modified-on   CDATA #IMPLIED\n"
    "    \">\n"
    "<!ENTITY % modificationtime \"\n"
    "    modified-on   CDATA #IMPLIED\n"
    "    \">\n"
    "<!ELEMENT root ((node*|template?|editor?)*)>\n"
    "<!ATTLIST root\n"
    "    version       CDATA #REQUIRED\n"
    "    %creation;\n"
    "    %modification;\n"
    ">\n";

const static char*      dtd_2 =
    "<!ELEMENT comment (#PCDATA)>\n"
    "<!ATTLIST comment\n"
    "    %creation;\n"
    "    %modification;\n"
    ">\n"
    "<!ELEMENT editor (user*)>\n"
    "<!ATTLIST editor\n"
    "    %creationtime;\n"
    ">\n"
    "<!ELEMENT node ((comment?|node*)*)>\n"
    "<!ATTLIST node\n"
    "    %creation;\n"
    "    %modification;\n"
    "    label         CDATA #REQUIRED\n"
    ">\n"
    "<!ELEMENT template (title*)>\n"
    "<!ATTLIST template\n"
    "    %creationtime;\n"
    "    %modification;\n"
    ">\n";

const static char*      dtd_3 =
    "<!ELEMENT title (#PCDATA)>\n"
    "<!ATTLIST title\n"
    "    %creationtime;\n"
    "    %modificationtime;\n"
    "    level         CDATA #REQUIRED\n"
    ">\n"
    "<!ELEMENT user (#PCDATA)>\n"
    "<!ATTLIST user\n"
    "    uid           CDATA #REQUIRED\n"
    "    %creationtime;\n"
    ">\n";


/* #############################################################################
 *
 * Description    add a DTD to the xml document
 * Author         Harry Brueckner
 * Date           2005-05-04
 * Arguments      SHOWERROR_FN - callback function for error messages
 * Return         -1 if the DTD could not be updated, 0 if the document did not
 *                validate and 1 if everything is ok
 */
int checkDtd(SHOWERROR_FN showerror_cb)
  {
    xmlDtd*             dtd;
    xmlParserInputBuffer*   inputbuffer;
    xmlValidCtxt*       context = xmlNewValidCtxt();
    char*               dtdbuffer;

    TRACE(99, "checkDtd()", NULL);

    if (!xmldoc)
      { return -1; }

    xmlRemoveDtd();

    /* first we create the DTD as a whole */
    dtdbuffer = memAlloc(__FILE__, __LINE__,
        strlen(dtd_1) + strlen(dtd_2) + strlen(dtd_3) + 1);
    if (!dtdbuffer)
      { return -1; }

    strStrncpy(dtdbuffer, dtd_1, strlen(dtd_1) + 1);
    strStrncat(dtdbuffer, dtd_2, strlen(dtd_2) + 1);
    strStrncat(dtdbuffer, dtd_3, strlen(dtd_3) + 1);

    /* create the xml buffer */
    inputbuffer = xmlParserInputBufferCreateMem(dtdbuffer, strlen(dtdbuffer),
        XML_CHAR_ENCODING_8859_1);
    memFreeString(__FILE__, __LINE__, dtdbuffer);
    if (!dtdbuffer)
      { return -1; }

    /* and finally create the DTD */
    dtd = xmlIOParseDTD(NULL, inputbuffer, XML_CHAR_ENCODING_8859_1);

    if (!dtd)
      { return -1; }

    /* and attach the DTD to the document */
    if (!xmldoc -> children)
      { xmlAddChild((xmlNode*)xmldoc, (xmlNode*)dtd); }
    else
      { xmlAddPrevSibling(xmldoc -> children, (xmlNode*)dtd); }

    /* we set our own error handler */
    validateShowError = showerror_cb;
    context -> error = xmlValidateError;
    context -> warning = xmlValidateWarning;

    return xmlValidateDtd(context, xmldoc, dtd);
  }


/* #############################################################################
 *
 * Description    free the XML parser stuff
 * Author         Harry Brueckner
 * Date           2005-03-18
 * Arguments      void
 * Return         void
 */
void freeXML(void)
  {
    TRACE(99, "freeXML()", NULL);

    if (xmldoc)
      { xmlFreeDoc(xmldoc); }

    /* cleanup function for the XML library. */
    xmlCleanupParser();

    /* this is to debug memory for regression tests */
    xmlMemoryDump();
  }


/* #############################################################################
 *
 * Description    initialize the XML parser
 * Author         Harry Brueckner
 * Date           2005-03-18
 * Arguments      void
 * Return         void
 */
void initXML(void)
  {
    TRACE(99, "initXML()", NULL);

    xmldoc = NULL;

    /* this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION
  }


/* #############################################################################
 *
 * Description    read, decrypt and parse the given filename
 * Author         Harry Brueckner
 * Date           2005-03-18
 * Arguments      char* filename  - filename to read
 *                char** errormsg - pointer to the GpgMe error message, if any
 *                PASSPHRASE_FN   - passphrase callback function
 *                SHOWERROR_FN    - callback function for error messages
 * Return         1 on error, otherwise 0
 */
int xmlDataFileRead(char* filename, char** errormsg,
    PASSPHRASE_FN passphrase_cb, SHOWERROR_FN showerror_cb)
  {
    xmlNode*            node;
    struct stat         filestat;
    off_t               size;
    int                 error = 0,
                        fd,
                        gpgsize = 0,
                        validate;
    char*               buffer = NULL;
    char*               gpgbuffer = NULL;
    char*               tmpbuffer = NULL;

    TRACE(99, "xmlDataFileRead()", NULL);

    /* we initialize the error message */
    *errormsg = NULL;

    fd = fileLockOpen(filename, O_RDONLY, -1, &tmpbuffer);
    if (fd == -1)
      {   /* unable to read the file, we start a new one */
        showerror_cb(_("file error"), tmpbuffer);
        memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

        if (fileExists(filename))
          {   /* if the file exists and we can't read it it's a hard error and
               * we must not continue; we don't need another error message since
               * we already had one displayed.
               */
            return 1;
          }

        /* we create a new, empty document */
        xmldoc = xmlNewDoc(BAD_CAST "1.0");
        node = xmlNewDocNode(xmldoc, NULL, BAD_CAST "root", NULL);
        xmlDocSetRootElement(xmldoc, node);
        createEditorsNode();
        createTemplateNode();

        /* we insert the current version and also create the created and
         * modified entries.
         */
        xmlVersionUpdate(1);

        checkDtd(showerror_cb);

        keyDefaults();

        /* we created a new document, so it's changed */
        runtime -> datachanged = 1;
      }
    else if (!fstat(fd, &filestat) &&
        filestat.st_size)
      {   /* we found a file and have to read it */
        size = filestat.st_size;
        buffer = memAlloc(__FILE__, __LINE__, size);
        /* Flawfinder: ignore */
        if (read(fd, buffer, size) != size)
          {
            memFree(__FILE__, __LINE__, buffer, size);
            return 1;
          }
        close(fd);

        if (config -> encryptdata)
          {
            error = gpgDecrypt(buffer, size, &gpgbuffer, &gpgsize,
                passphrase_cb, showerror_cb);
            if (error)
              { *errormsg = _("could not decrypt database file."); }

            /* since we decrypted the file, we swap buffers */
            memFree(__FILE__, __LINE__, buffer, size);
            buffer = gpgbuffer;
            size = gpgsize;

#ifdef TEST_OPTION
            if (config -> testrun &&
                !strcmp("decrypt", config -> testrun) &&
                buffer)
              { printf("%s", buffer); }
#endif

            /* TODO: add the same keys as used in the encrypted file;
             *       For now we just always add the default keys
             */
            if (runtime -> realmhint)
              {   /* if we have a realm, we add it to the default keys since we
                   * probably want to encrypt data for ourselves as well
                   */
                config -> defaultkeys = listAdd(config -> defaultkeys,
                    runtime -> realmhint);
              }

            keyDefaults();
          }
        else
          {
            /* if we run in unencrypted mode, we must add all default keys */
            keyDefaults();

            showerror_cb(_("warning"), _("the database file is read in unecrypted mode."));
          }

        if (!error &&
            buffer[0] == '\x1f' && buffer[1] == '\x8b')
          {   /* we have a gzipped buffer and must compress it */
            error = zlibDecompress(buffer, size, &gpgbuffer, &gpgsize,
                errormsg);
            if (error)
              {
                tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
                snprintf(tmpbuffer, STDBUFFERLENGTH,
                    _("error (%s) compressing file '%s'."),
                    *errormsg,
                    filename);
                showerror_cb(_("compression error"), tmpbuffer);
                memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

                memFree(__FILE__, __LINE__, buffer, size);
                return 1;
              }

            memFree(__FILE__, __LINE__, buffer, size);
            buffer = gpgbuffer;
            size = gpgsize;
          }
        else
          {
            if (!error && config -> compression > 0)
              {
                tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
                snprintf(tmpbuffer, STDBUFFERLENGTH,
                    _("database '%s' was not compressed."),
                    filename);
                showerror_cb(_("warning"), tmpbuffer);
                memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);
              }
          }

        if (!error)
          {
            xmldoc = xmlReadMemory(buffer, size, filename, config -> encoding,
                XML_PARSE_PEDANTIC | XML_PARSE_NONET | XML_PARSE_NOCDATA);
            if (!xmldoc)
              {
                memFree(__FILE__, __LINE__, buffer, size);

                tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
                snprintf(tmpbuffer, STDBUFFERLENGTH,
                    _("failed to parse xml document '%s'."),
                    filename);
                showerror_cb(_("file error"), tmpbuffer);
                memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

                return 1;
              }
          }

        if (buffer)
          { memFree(__FILE__, __LINE__, buffer, size); }

        /* we update our document version */
        if (!error)
          { xmlVersionUpdate(0); }

        if (!error)
          {
            validate = checkDtd(showerror_cb);

            if (validate != 1)
              {
                tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
                snprintf(tmpbuffer, STDBUFFERLENGTH,
                    _("failed to validate xml document '%s'."),
                    filename);
                showerror_cb(_("file error"), tmpbuffer);
                memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

                return 1;
              }
          }
      }
    else
      {   /* the given file has size 0 */
        error = 1;
      }

    return error;
  }


/* #############################################################################
 *
 * Description    encrypt and write our data to the given filename
 * Author         Harry Brueckner
 * Date           2005-03-18
 * Arguments      char* filename  - filename to write to
 *                char** errormsg - pointer to the GpgMe error message, if any
 *                PASSPHRASE_FN   - passphrase callback function
 *                SHOWERROR_FN    - callback function for error messages
 * Return         1 on error, otherwise 0
 */
int xmlDataFileWrite(char* filename, char** errormsg,
    PASSPHRASE_FN passphrase_cb, SHOWERROR_FN showerror_cb)
  {
    xmlNode*            rootnode;
    xmlChar*            xmlbuffer = NULL;
    int                 error = 0,
                        fd,
                        gpgsize = 0,
                        size;
    char*               buffer = NULL;
    char*               gpgbuffer = NULL;
    char*               tmpbuffer = NULL;


    /* we initialize the error message */
    *errormsg = NULL;

    /* update the modification date of the root node */
    rootnode = xmlDocGetRootElement(xmldoc);
    if (rootnode)
      { xmlSetModification(rootnode); }

    /* we create the memory buffer */
    xmlDocDumpMemoryEnc(xmldoc, &xmlbuffer, &size, config -> encoding);
    buffer = (char*)xmlbuffer;

    if (buffer && !error && config -> encryptdata)
      {   /* we have a buffer and must compress it */
        error = zlibCompress(buffer, size, &gpgbuffer, &gpgsize, errormsg);
        if (error)
          {
            tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
            snprintf(tmpbuffer, STDBUFFERLENGTH,
                _("error (%s) compressing file '%s'."),
                *errormsg,
                filename);
            showerror_cb(_("compression error"), tmpbuffer);
            memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

            memFree(__FILE__, __LINE__, buffer, size);
            return 1;
          }

        xmlFree(buffer);
        buffer = gpgbuffer;
        size = gpgsize;
      }
    else if (buffer && !error && !config -> encryptdata)
      {   /* we don't have to encrypt data, so nothing is compressed */
        gpgbuffer = memAlloc(__FILE__, __LINE__, size);
        /* Flawfinder: ignore */
        memcpy(gpgbuffer, buffer, size);

        xmlFree(buffer);
        buffer = gpgbuffer;
      }

    if (buffer && !error &&
        config -> encryptdata)
      {
        error = gpgEncrypt(buffer, size, &gpgbuffer, &gpgsize, passphrase_cb,
            showerror_cb);
        if (error)
          { *errormsg = _("could not encrypt database file."); }

        memFree(__FILE__, __LINE__, buffer, size);
        buffer = gpgbuffer;
        size = gpgsize;
      }
    else
      {
        showerror_cb(_("warning"),
            _("the database file is written in unecrypted mode."));
      }

    if (buffer && !error)
      {   /* if we have a buffer we write the file */
        createBackupfile(filename, showerror_cb);

        fd = fileLockOpen(filename, O_WRONLY | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, &tmpbuffer);
        if (fd == -1)
          {   /* error opening the file */
            showerror_cb(_("file error"), tmpbuffer);
            memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

            *errormsg = strerror(errno);

            memFree(__FILE__, __LINE__, buffer, size);
            return 1;
          }

        if (write(fd, buffer, size) != size)
          {   /* error writing the file */
            tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
            snprintf(tmpbuffer, STDBUFFERLENGTH,
                _("error %d (%s) writing file '%s'."),
                errno,
                strerror(errno),
                filename);
            showerror_cb(_("file error"), tmpbuffer);
            memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

            *errormsg = strerror(errno);

            memFree(__FILE__, __LINE__, buffer, size);
            return 1;
          }

        lockf(fd, F_UNLCK, 0L);
        close(fd);
      }

    if (buffer && size)
      { memFree(__FILE__, __LINE__, buffer, size); }

    return error;
  }


/* #############################################################################
 *
 * Description    encode the XML entities in the given string
 * Author         Harry Brueckner
 * Date           2005-07-08
 * Arguments      xmlChar* string   - string to encode the entities
 * Return         xmlChar* encoded string
 */
xmlChar* xmlEncodeCommentEntities(xmlChar* string)
  {
    TRACE(99, "xmlEncodeCommentEntities()", NULL);

    return xmlEncodeEntitiesReentrant(xmldoc, string);
  }


/* #############################################################################
 *
 * Description    get the documents root node
 * Author         Harry Brueckner
 * Date           2005-04-18
 * Arguments      void
 * Return         xmlNode* node pointer to the root element
 */
xmlNode* xmlGetDocumentRoot(void)
  {
    TRACE(99, "xmlGetDocumentRoot()", NULL);

    if (xmldoc)
      { return xmlDocGetRootElement(xmldoc); }
    else
      { return NULL; }
  }


/* #############################################################################
 *
 * Description    remove all DTDs from the XML document
 * Author         Harry Brueckner
 * Date           2005-05-06
 * Arguments      void
 * Return         void
 */
void xmlRemoveDtd(void)
  {
    xmlNode*            curnode;
    xmlNode*            delnode = NULL;

    TRACE(99, "xmlRemoveDtd()", NULL);

    if (!xmldoc)
      { return; }

    /* we delete all DTDs in the document */
    curnode = xmldoc -> children;
    while (curnode)
      {
        if (curnode -> type == XML_DTD_NODE)
          { delnode = curnode; }

        curnode = curnode -> next;

        if (delnode)
          {
            xmlUnlinkNode(delnode);
            delnode = NULL;
          }
      }
  }


/* #############################################################################
 *
 * Description    error/warning callback for DTD validation
 * Author         Harry Brueckner
 * Date           2005-05-06
 * Arguments      void* context     - the validation context
 *                const char* msg   - format message
 *                ...               - arguments for the message
 * Return         void
 */
void xmlValidateError(void* context, const char* msg, ...)
  {
    va_list             list;
    int                 size;
    char*               tmpbuffer;

    TRACE(99, "xmlValidateError()", NULL);

    tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);

    va_start(list, msg);
    /* Flawfinder: ignore */
    vsnprintf(tmpbuffer, STDBUFFERLENGTH, msg, list);
    va_end(list);

    /* the message has a \n at the end so we remove it */
    size = strlen(tmpbuffer);
    if (size > 0 && size < STDBUFFERLENGTH)
      { tmpbuffer[size - 1] = 0; }

    validateShowError(_("validation error"), tmpbuffer);

    memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);
  }


/* #############################################################################
 *
 * Description    error/warning callback for DTD validation
 * Author         Harry Brueckner
 * Date           2005-05-06
 * Arguments      void* context     - the validation context
 *                const char* msg   - format message
 *                ...               - arguments for the message
 * Return         void
 */
void xmlValidateWarning(void* context, const char* msg, ...)
  {
    va_list             list;
    int                 size;
    char*               tmpbuffer;

    TRACE(99, "xmlValidateWarning()", NULL);

    tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);

    va_start(list, msg);
    /* Flawfinder: ignore */
    vsnprintf(tmpbuffer, STDBUFFERLENGTH, msg, list);
    va_end(list);

    /* the message has a \n at the end so we remove it */
    size = strlen(tmpbuffer);
    if (size > 0 && size < STDBUFFERLENGTH)
      { tmpbuffer[size - 1] = 0; }

    validateShowError(_("validation warning"), tmpbuffer);

    memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);
  }


/* #############################################################################
 *
 * Description    walk the tree and update all nodes
 * Author         Harry Brueckner
 * Date           2005-04-04
 * Arguments      long oldversion     - the version to convert from
 *                xmlNode* rootnode   - the node to walk
 * Return         void
 */
void xmlVersionNodeUpdate(long oldversion, xmlNode* rootnode)
  {
    xmlNode*            curnode;

    TRACE(99, "xmlVersionNodeUpdate()", NULL);

    if (!rootnode)
      { return; }

    curnode = rootnode -> children;
    while (curnode)
      {
        if (curnode -> type == XML_ELEMENT_NODE)
          {
            if (oldversion < 0x000000002)
              {   /* we upgrade from 'unknown' version; before 0.2alpha,
                   * the version attribute did not exist
                   */
                xmlSetCreation(curnode);
              }
          }

        xmlVersionNodeUpdate(oldversion, curnode);

        curnode = curnode -> next;
      }
  }


/* #############################################################################
 *
 * Description    check if our current document has the correct version, if not
 *                update it
 * Author         Harry Brueckner
 * Date           2005-04-04
 * Arguments      xmlNode* node   - the node to walk
 * Return         void
 */
void xmlVersionUpdate(int silent)
  {
    static char*        curversion = PACKAGE_VERSION;
    xmlNode*            rootnode;
    xmlChar*            prop;
    unsigned long       version;
    int                 size;
    char*               old;
    char*               ptr;

    TRACE(99, "xmlVersionUpdate()", NULL);

    rootnode = xmlDocGetRootElement(xmldoc);
    if (!rootnode)
      { return; }

    /* get the current version of the document */
    prop = xmlGetProp(rootnode, BAD_CAST "version");
    if (prop && !strcmp(curversion, (char*)prop))
      { return; }

    if (!prop)
      {   /* we do not even have a version info, so we add a new one */
        xmlVersionNodeUpdate(0, rootnode);

        xmlNewProp(rootnode, BAD_CAST "version", BAD_CAST curversion);
        xmlSetCreation(rootnode);
      }
    else
      {   /* we have version information but its not up to date */
        size = strlen((char*)prop) + 1;
        old = memAlloc(__FILE__, __LINE__, size);
        strStrncpy(old, (char*)prop, size);

        ptr = old;
        while (*ptr && *ptr != '.')
          { ptr++; }
        if (*ptr == '.')
          {
            *ptr = '\x0';
          }
        /* Flawfinder: ignore */
        version = (atoi(old) << 16) + atoi(ptr + 1);

        xmlSetProp(rootnode, BAD_CAST "version", BAD_CAST curversion);
        xmlVersionNodeUpdate(version, rootnode);


        memFree(__FILE__, __LINE__, old, size);
      }

    /* we update the modified timestamp */
    xmlSetModification(rootnode);

    runtime -> datachanged = 1;
    if (!silent)
      { runtime -> updatestatus = 1; }
  }


/* #############################################################################
 */

