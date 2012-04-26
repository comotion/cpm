/* #############################################################################
 * interface to the xml library's UFT-8 conversion
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
#include "general.h"
#include "interface_utf8.h"
#include "memory.h"
#include "string.h"


/* #############################################################################
 * global variables
 */
xmlCharEncodingHandler* encodingHandler = NULL;
char*                   convertbuffer = NULL;


/* #############################################################################
 *
 * Description    convert the given string to the terminal display type (latin1)
 *                or UTF-8
 * Author         Harry Brueckner
 * Date           2005-07-03
 * Arguments      int direction   - 0 for input (to UTF-8),
 *                                  1 for output direction
 *                char* instring  - string to convert
 * Return         converted string
 */
char* convert(int direction, char* instring)
  {
    int                 insize,
                        outsize,
                        ret,
                        tmp;

    TRACE(199, "convert()", NULL);

    if (!instring)
        return NULL;
    if (!encodingHandler || !encodingHandler->output)
       return instring; // NULL handler, return instring

    if (convertbuffer)
      {
        memFreeString(__FILE__, __LINE__, convertbuffer);
        convertbuffer = NULL;
      }

    insize = strlen((char*)instring) + 1;
    outsize = insize * 2 - 1;
    convertbuffer = memAlloc(__FILE__, __LINE__, outsize);
    // (xml)encodingHandler doesn't handle empty strings
    if(insize == 1){
       convertbuffer[0] = '\0';
       return convertbuffer;
    }

    tmp = insize - 1;
    if (direction)
      {
        ret = encodingHandler -> output(
            (unsigned char*)convertbuffer, &outsize,
            (unsigned char*)instring, &tmp);
      }
    else
      {
        ret = encodingHandler -> input(
            (unsigned char*)convertbuffer, &outsize,
            (unsigned char*)instring, &tmp);
      }
    if (ret <= 0 || tmp != insize - 1)
      {
        fprintf(stderr, _("conversion failed for string '%s' (%d).\n"),
            instring,
            ret);
        memFree(__FILE__, __LINE__, convertbuffer, outsize);
        return NULL;
      }
    else
      {
        convertbuffer = memRealloc(__FILE__, __LINE__,
            convertbuffer, insize * 2 - 1, outsize + 1);
        convertbuffer[outsize] = 0;
      }

    return convertbuffer;
  }


/* #############################################################################
 *
 * Description    convert the given string to the terminal display type
 * Arguments      char* instring  - string to convert
 * Return         converted string
 */
char* convert2terminal(xmlChar* instring)
  {
    TRACE(199, "convert2terminal()", NULL);

    return convert(1, (char*)instring);
  }


/* #############################################################################
 *
 * Description    convert the given string to UTF8 (xml)
 * Author         Harry Brueckner
 * Date           2005-07-01
 * Arguments      char* instring  - string to convert
 * Return         converted string
 */
xmlChar* convert2xml(char* instring)
  {
    TRACE(199, "convert2xml()", NULL);

    return (xmlChar*)convert(0, instring);
  }


/* #############################################################################
 *
 * Description    free the UTF8 module
 * Author         Harry Brueckner
 * Date           2005-03-18
 * Arguments      void
 * Return         void
 */
void freeUTF8Interface(void)
  {
    TRACE(99, "freeUTF8Interface()", NULL);

    if (convertbuffer)
      {
        memFreeString(__FILE__, __LINE__, convertbuffer);
        convertbuffer = NULL;
      }

    if (encodingHandler)
      { xmlCharEncCloseFunc(encodingHandler); }
  }


/* #############################################################################
 *
 * Description    initialize the XML encoding engine
 * Author         Harry Brueckner
 * Date           2005-07-01
 * Arguments      char* encoding  - the encoding string
 * Return         0 on error or 1 if all is well
 */
#define TSTRING         "teststring äöü ÄÖÜ"
int initUTF8Encoding(char* encoding)
  {
    xmlCharEncoding     encoder;
    int                 status = 0;
    char*               tmp;
    char*               tstring1 = NULL;
    char*               tstring2 = NULL;

    TRACE(99, "initUTF8Encoding()", NULL);

    /* we first try to find the encode by it's name */
    encoder = xmlParseCharEncoding(encoding);
    if (encoder <= 0)
      {fprintf(stderr,"parse char\n"); return 0; }

    /* and then load it by using it's id */
    encodingHandler = xmlGetCharEncodingHandler(encoder);

    if (!encodingHandler) {
        switch(encoder) {
            case XML_CHAR_ENCODING_NONE:
            case XML_CHAR_ENCODING_UTF8:
                // null handler, no encoder.
                return 1;
            default:
                fprintf(stderr,"Failed to init character handler.\n"); 
                return 0;
        }
    }

    tmp = convert(0, TSTRING);
    if (tmp)
      {   /* converting to UTF-8 was successful */
        tstring1 = memAlloc(__FILE__, __LINE__, strlen(tmp) + 1);
        strStrncpy(tstring1, tmp, strlen(tmp) + 1);
      }

    tmp = convert(1, tstring1);
    if (tmp)
      {   /* converting to terminal usage was successful */
        tstring2 = memAlloc(__FILE__, __LINE__, strlen(tmp) + 1);
        strStrncpy(tstring2, tmp, strlen(tmp) + 1);
      }

    if (tstring1 &&
        tstring2 &&
        !strcmp(TSTRING, tstring2))
      { status = 1; }

    memFreeString(__FILE__, __LINE__, tstring1);
    memFreeString(__FILE__, __LINE__, tstring2);

    return status;
  }


/* #############################################################################
 */

