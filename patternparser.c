/* #############################################################################
 * code for handling the pattern parsing for the search patterns.
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
#include "configuration.h"
#include "general.h"
#include "interface_xml.h"
#include "listhandler.h"
#include "memory.h"
#include "patternparser.h"
#include "string.h"


/* #############################################################################
 * global variables
 */
SEARCHPATTERN**         patterndata;
SEARCHPATTERN**         resultdata;
int                     patterncount;


/* #############################################################################
 * internal functions
 */
int getPatternString(SEARCHPATTERN* pattern, char** path, char** string);
int patternCreate(char* patternstring, SEARCHPATTERN* pattern);
void patternDump(SEARCHPATTERN* pattern);
int patternTemplateId(char* template);


/* #############################################################################
 *
 * Description    free the pattern parser
 * Author         Harry Brueckner
 * Date           2005-04-06
 * Arguments      void
 * Return         void
 */
void freePatternparser(void)
  {
    SEARCHPATTERN*      cur;
    SEARCHPATTERN*      next;
    int                 i;

    TRACE(99, "freePatternparser()", NULL);

    if (patterndata || resultdata)
      {
        for (i = 0; i < patterncount; i++)
          {
#ifdef TEST_OPTION
            if (config -> testrun &&
                !strcmp("searchpattern", config -> testrun))
              {
                patternDump(patterndata[i]);
                patternDump(resultdata[i]);
              }
#endif

            cur = patterndata[i];
            while (cur)
              {
                next = cur -> next;
                if (cur -> type == PATTERN_STRING)
                  { memFreeString(__FILE__, __LINE__, cur -> string); }
                memFree(__FILE__, __LINE__, cur, sizeof(SEARCHPATTERN));
                cur = next;
              }

            cur = resultdata[i];
            while (cur)
              {
                next = cur -> next;
                if (cur -> type == PATTERN_STRING)
                  { memFreeString(__FILE__, __LINE__, cur -> string); }
                memFree(__FILE__, __LINE__, cur, sizeof(SEARCHPATTERN));
                cur = next;
              }
          }

        memFree(__FILE__, __LINE__,
            patterndata, sizeof(SEARCHPATTERN*) * patterncount);
        memFree(__FILE__, __LINE__,
            resultdata, sizeof(SEARCHPATTERN*) * patterncount);
      }
  }


/* #############################################################################
 *
 * Description    get a specific result pattern filled out
 * Author         Harry Brueckner
 * Date           2005-04-12
 * Arguments      int id        - id of the search pattern
 *                char** path   - path information to full into the pattern
 *                char* string  - result string which must be freed by the
 *                                caller
 * Return         1 on error, 0 if ok
 */
int getPatternResultString(int id, char** path, char** string)
  {
    TRACE(99, "getPatternResultString()", NULL);

    return getPatternString(resultdata[id], path, string);
  }


/* #############################################################################
 *
 * Description    get a specific search pattern filled out
 * Author         Harry Brueckner
 * Date           2005-04-12
 * Arguments      int id        - id of the search pattern
 *                char** path   - path information to full into the pattern
 *                char* string  - result string which must be freed by the
 *                                caller
 * Return         1 on error, 0 if ok
 */
int getPatternSearchString(int id, char** path, char** string)
  {
    TRACE(99, "getPatternSearchString()", NULL);

    return getPatternString(patterndata[id], path, string);
  }


/* #############################################################################
 *
 * Description    create a string from a searchpattern and path information
 * Author         Harry Brueckner
 * Date           2005-04-12
 * Arguments      SEARCHPATTERN pattern   - pattern to create
 *                char** path             - path information to full into the
 *                                          pattern
 *                char* string            - result string which must be freed
 *                                          by the caller
 * Return         1 on error, 0 if ok
 */
int getPatternString(SEARCHPATTERN* pattern, char** path, char** string)
  {
    SEARCHPATTERN*      cpattern = pattern;
    int                 maxlevel = listCount(path);
    char*               concat;

    TRACE(99, "getPatternString()", NULL);

    *string = NULL;
    while (cpattern)
      {
        switch (cpattern -> type)
          {
            case PATTERN_TEMPLATE:
                if (cpattern -> templateid > maxlevel)
                  { return 1; }
                concat = path[cpattern -> templateid - 1];
                break;
            case PATTERN_STRING:
                concat = cpattern -> string;
                break;
            default:
                concat = NULL;
          }

        if (concat)
          {   /* we have a string to concat to the result */
            if (!*string)
              {   /* new string */
                *string = memAlloc(__FILE__, __LINE__, strlen(concat) + 1);
                *string[0] = 0;
              }
            else
              {   /* resize the string */
                *string = memRealloc(__FILE__, __LINE__,
                    *string,
                    strlen(*string) + 1,
                    strlen(*string) + strlen(concat) + 1);
              }

            strStrncat(*string, concat, strlen(concat) + 1);
          }

        cpattern = cpattern -> next;
      }

    return 0;
  }


/* #############################################################################
 *
 * Description    initialize the pattern parser
 * Author         Harry Brueckner
 * Date           2005-04-06
 * Arguments      void
 * Return         void
 */
void initPatternparser(void)
  {
    TRACE(99, "initPatternparser()", NULL);

    patterndata = NULL;
    resultdata = NULL;
    patterncount = 0;
  }


/* #############################################################################
 *
 * Description    create one pattern structure
 * Author         Harry Brueckner
 * Date           2005-04-08
 * Arguments      char* patternstring     - the string to parse
 *                SEARCHPATTERN* pattern  - the parsed pattern
 * Return         0 if everything is ok, otherwise 1
 */
#define PATTERNLENGTH 1024
int patternCreate(char* patternstring, SEARCHPATTERN* pattern)
  {
    SEARCHPATTERN*      cpattern = NULL;
    int                 error = 0,
                        escaped = 0,
                        intag = 0,
                        size = 0;
    char*               cbuffer;
    char*               ptr;

    TRACE(99, "patternCreate()", NULL);

    if (!patternstring)
        return 1;

    cbuffer = memAlloc(__FILE__, __LINE__, PATTERNLENGTH);

    ptr = patternstring;
    while (*ptr && size < PATTERNLENGTH && !error)
      {
        if (escaped)
          {   /* the current character was escaped, so we don't interpret it */
            cbuffer[size++] = *ptr;
            escaped = 0;
          }
        else
          {
            switch (*ptr)
              {
                case '<':   /* a tag starts */
                    if (intag)
                      {   /* error, we are already in a tag */
                        fprintf(stderr,
                            _("error: tag not closed in pattern '%s'.\n"),
                            patternstring);
                        error = 1;
                      }
                    else
                      {
                        if (size)
                          {   /* handle the last string, if there is any */
                            cbuffer[size] = 0;

                            if (!cpattern)
                              { cpattern = pattern; }
                            else
                              {
                                cpattern -> next = memAlloc(__FILE__, __LINE__,
                                    sizeof(SEARCHPATTERN));
                                cpattern = cpattern -> next;
                                cpattern -> templateid = -1;
                                cpattern -> next = NULL;
                              }


                            cpattern -> type = PATTERN_STRING;
                            cpattern -> string = memAlloc(__FILE__, __LINE__,
                                strlen(cbuffer) + 1);
                            strStrncpy(cpattern -> string, cbuffer,
                                strlen(cbuffer) + 1);
                          }

                        intag = 1;
                        size = 0;
                      }
                    break;
                case '>':   /* a tag ends */
                    if (!intag)
                      {   /* error, we are not in a tag */
                        fprintf(stderr, 
                            _("error: tag not opened in pattern '%s'.\n"),
                            patternstring);
                        error = 1;
                      }
                    else
                      {
                        if (size)
                          {   /* handle the tag entry */
                            cbuffer[size] = 0;

                            if (!cpattern)
                              { cpattern = pattern; }
                            else
                              {
                                cpattern -> next = memAlloc(__FILE__, __LINE__,
                                    sizeof(SEARCHPATTERN));
                                cpattern = cpattern -> next;
                                cpattern -> string = NULL;
                                cpattern -> next = NULL;
                              }


                            cpattern -> type = PATTERN_TEMPLATE;
                            cpattern -> templateid = patternTemplateId(cbuffer);
                            if (cpattern -> templateid == -1)
                              {
                                fprintf(stderr,
                                    _("error: unknown template '%s' in pattern '%s'.\n"),
                                    cbuffer, patternstring);
                                error = 1;
                              }
                          }

                        intag = 0;
                        size = 0;
                      }
                    break;
                case '\\':  /* something is escaped */
                    escaped = 1;
                    break;
                default:    /* any other character */
                    cbuffer[size++] = *ptr;
                    break;
              }
          }

        ptr++;
      }
    if (intag)
      {
        fprintf(stderr, _("error: tag not closed in pattern '%s'.\n"),
            patternstring);
        error = 1;
      }
    if (!error && escaped)
      {
        fprintf(stderr, _("error: string not terminated in pattern '%s'.\n"),
            patternstring);
        error = 1;
      }
    if (!error && size)
      {   /* handle the last string, if there is any */
        cbuffer[size] = 0;

        if (!cpattern)
          { cpattern = pattern; }
        else
          {
            cpattern -> next = memAlloc(__FILE__, __LINE__,
                sizeof(SEARCHPATTERN));
            cpattern = cpattern -> next;
            cpattern -> templateid = -1;
            cpattern -> next = NULL;
          }


        cpattern -> type = PATTERN_STRING;
        cpattern -> string = memAlloc(__FILE__, __LINE__, strlen(cbuffer) + 1);
        strStrncpy(cpattern -> string, cbuffer, strlen(cbuffer) + 1);
      }

    if (size >= PATTERNLENGTH)
      {
        fprintf(stderr, _("error: pattern too long ('%s').\n"), patternstring);
        error = 1;
      }

    if (cbuffer)
      { memFree(__FILE__, __LINE__, cbuffer, PATTERNLENGTH); }

    return error;
  }
#undef PATTERNLENGTH


/* #############################################################################
 *
 * Description    dump the given pattern
 * Author         Harry Brueckner
 * Date           2005-04-11
 * Arguments      SEARCHPATTERN* pattern  - pattern to dump
 * Return         void
 */
void patternDump(SEARCHPATTERN* pattern)
  {
    SEARCHPATTERN*      cur = pattern;

    TRACE(99, "patternDump()", NULL);

    printf("pattern");
    while (cur)
      {
        switch (cur -> type)
          {
            case PATTERN_UNDEF:
                printf(" -> undef ");
                break;
            case PATTERN_STRING:
                printf(" -> string '%s'", cur -> string);
                break;
            case PATTERN_TEMPLATE:
                printf(" -> template %d", cur -> templateid);
                break;
          }

        cur = cur -> next;
      }
    printf("\n");
  }


/* #############################################################################
 *
 * Description    parse the defined patterns for later use
 * Author         Harry Brueckner
 * Date           2005-04-06
 * Arguments      void
 * Return         0 if everything is ok, otherwise 1
 */
int patternParse(void)
  {
    int                 error = 0,
                        i;

    TRACE(99, "patternParse()", NULL);

    patterncount = listCount(runtime -> searchpatterns);
    patterndata = memAlloc(__FILE__, __LINE__,
        sizeof(SEARCHPATTERN*) * patterncount);
    resultdata = memAlloc(__FILE__, __LINE__,
        sizeof(SEARCHPATTERN*) * patterncount);

    for (i = 0; i < patterncount; i++)
      {
        patterndata[i] = memAlloc(__FILE__, __LINE__,
            sizeof(SEARCHPATTERN));
        patterndata[i] -> type = PATTERN_UNDEF;
        patterndata[i] -> templateid = -1;
        patterndata[i] -> string = NULL;
        patterndata[i] -> next = NULL;

        if (patternCreate(runtime -> searchpatterns[i], patterndata[i]))
          { error = 1; }

        resultdata[i] = memAlloc(__FILE__, __LINE__,
            sizeof(SEARCHPATTERN));
        resultdata[i] -> type = PATTERN_UNDEF;
        resultdata[i] -> templateid = -1;
        resultdata[i] -> string = NULL;
        resultdata[i] -> next = NULL;

        if (patternCreate(runtime -> resultpatterns[i], resultdata[i]))
          { error = 1; }
      }

    return error;
  }


/* #############################################################################
 *
 * Description    find the id for the given template string
 * Author         Harry Brueckner
 * Date           2005-04-11
 * Arguments      char* template  - the name of the template
 * Return         int id of the template or -1 on error
 */
int patternTemplateId(char* template)
  {
    int                 i,
                        id;

    TRACE(99, "patternTemplateId()", NULL);

    /* first we look into the database templates */
    id = xmlInterfaceTemplateGetId(template);
    if (id != -1)
      { return id; }

    /* if nothing is found so far, we look in the default templates */
    for (i = listCount(config -> defaulttemplates) - 1; i >= 0; i--)
      {
        if (!strcmp(template, config -> defaulttemplates[i]))
            return i + 1;
      }

    return -1;
  }


/* #############################################################################
 */

