/* #############################################################################
 * code for string handling
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
#ifdef HAVE_LIBZ
  #include <zlib.h>
#endif
#include "configuration.h"
#include "general.h"
#include "memory.h"
#include "zlib.h"


/* #############################################################################
 * global variables
 */
#define BUFFERSIZE      10240


/* #############################################################################
 *
 * Description    test function to see of if our zlib implementation works
 * Author         Harry Brueckner
 * Date           2005-05-19
 * Arguments      void
 * Return         void
 */
#ifdef TEST_OPTION
void testCompress(void)
  {
    static char*        testpattern = "this is harry's test";
    int                 c_size,
                        p_size,
                        size,
                        u_size;
    char*               buffer;
    char*               c_buffer;
    char*               errormsg;
    char*               u_buffer;

    TRACE(99, "testCompress()", NULL);

    /* each test tries a different buffer size*/
    if (!strcmp(config -> testrun, "compress1"))
      { size = 1048576; }
    else if (!strcmp(config -> testrun, "compress2"))
      { size = 524288; }
    else if (!strcmp(config -> testrun, "compress3"))
      { size = BUFFERSIZE + 1; }
    else if (!strcmp(config -> testrun, "compress4"))
      { size = BUFFERSIZE; }
    else if (!strcmp(config -> testrun, "compress5"))
      { size = 128; }
    else if (!strcmp(config -> testrun, "compress6"))
      { size = 32; }
    else
      { return; }

    buffer = memAlloc(__FILE__, __LINE__, size);

    /* we fill our test buffer */
    p_size = strlen(testpattern);
    u_size = 0;
    while (u_size + p_size + 1 < size)
      {
        /* Flawfinder: ignore */
        memcpy(buffer + u_size, testpattern, p_size);
        u_size += p_size;
      }

    zlibCompress(buffer, size, &c_buffer, &c_size, &errormsg);
    fprintf(stderr, "buffer size: %d -> %d\n", size, c_size);

    zlibDecompress(c_buffer, c_size, &u_buffer, &u_size, &errormsg);
    fprintf(stderr, "buffer size: %d -> %d\n", c_size, u_size);

    if (size == u_size)
      { fprintf(stderr, "buffer size ok\n"); }
    else
      { fprintf(stderr, "buffer size error\n"); }

    if (!memcmp(buffer, u_buffer, size))
      { fprintf(stderr, "buffer ok\n"); }
    else
      { fprintf(stderr, "buffer error\n"); }

    memFree(__FILE__, __LINE__, c_buffer, c_size);
    memFree(__FILE__, __LINE__, u_buffer, u_size);
    memFree(__FILE__, __LINE__, buffer, size);
  }
#endif


/* #############################################################################
 *
 * Description    compress a buffer
 * Author         Harry Brueckner
 * Date           2005-05-18
 * Arguments      char* srcbuffer   - source buffer
 *                int srclen        - length of the source buffer
 *                char** dstbuffer  - compressed buffer
 *                int* dstlen       - length of the compressed buffer
 * Return         1 on error, 0 on success
 */
int zlibCompress(char* srcbuffer, int srclen, char** dstbuffer, int* dstlen,
    char** errormsg)
  {
    Byte*               zbuffer;
    z_stream            zh;
    int                 error,
                        extrasize = srclen;

    TRACE(99, "zlibCompress()", NULL);

    *errormsg = NULL;

    /* we use this buffer for the compression */
    zbuffer = memAlloc(__FILE__, __LINE__, srclen + extrasize);

    zh.zalloc = (alloc_func)0;
    zh.zfree  = (free_func)0;
    zh.opaque = (voidpf)0;

    error = deflateInit2(&zh, config -> compression, Z_DEFLATED,
        15 + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (error != Z_OK)
      {
        *errormsg = zh.msg;
        memFree(__FILE__, __LINE__, zbuffer, srclen + extrasize);
        return 1;
      }

    zh.next_out   = zbuffer;
    zh.avail_out  = (uInt)srclen + extrasize;
    zh.next_in    = (Byte*)srcbuffer;
    zh.avail_in   = (uInt)srclen;

    do
      {
        if (extrasize > srclen)
          {   /* we increase the ouput buffer until it matches */
            zbuffer = memRealloc(__FILE__, __LINE__, zbuffer,
                srclen + extrasize - BUFFERSIZE, srclen + extrasize);
          }
        error = deflate(&zh, Z_FINISH);
        extrasize += BUFFERSIZE;
      }
    while (error == Z_OK);

    /* readjust the extrasize */
    extrasize -= BUFFERSIZE;

    if (error != Z_STREAM_END)
      {   /* Z_STREAM_END means everything is ok */
        *errormsg = zh.msg;
        memFree(__FILE__, __LINE__, zbuffer, srclen + extrasize);
        return 1;
      }

    /* we get the data back to the caller */
    *dstlen = zh.total_out;
    *dstbuffer = memAlloc(__FILE__, __LINE__, zh.total_out);
    /* Flawfinder: ignore */
    memcpy(*dstbuffer, zbuffer, zh.total_out);

    memFree(__FILE__, __LINE__, zbuffer, srclen + extrasize);

    error = deflateEnd(&zh);
    if (error != Z_OK)
      {
        *errormsg = zh.msg;
        return 1;
      }
    else
      { return 0; }
  }


/* #############################################################################
 *
 * Description    decompress a buffer
 * Author         Harry Brueckner
 * Date           2005-05-18
 * Arguments      char* srcbuffer   - source buffer
 *                int srclen        - length of the source buffer
 *                char** dstbuffer  - compressed buffer
 *                int* dstlen       - length of the compressed buffer
 * Return         1 on error, 0 on success
 */
int zlibDecompress(char* srcbuffer, int srclen, char** dstbuffer, int* dstlen,
    char** errormsg)
  {
    Byte*               zbuffer = NULL;
    z_stream            zh;
    int                 error,
                        offset = 0;

    TRACE(99, "zlibDecompress()", NULL);

    *errormsg = NULL;
    *dstbuffer = NULL;

    zh.zalloc = (alloc_func)0;
    zh.zfree  = (free_func)0;
    zh.opaque = (voidpf)0;

    zh.next_in  = (Byte*)srcbuffer;
    zh.avail_in = (uInt)srclen;

    error = inflateInit2(&zh, 15 + 16);
    if (error != Z_OK)
      {
        *errormsg = zh.msg;
        return 1;
      }

    zbuffer = memAlloc(__FILE__, __LINE__, BUFFERSIZE);

    while (1)
      {
        /* reset the zbuffer */
        zh.next_out = zbuffer;
        zh.avail_out = (uInt)BUFFERSIZE;

        error = inflate(&zh, Z_NO_FLUSH);

        if (zh.total_out)
          {
            if (*dstbuffer)
              {   /* we must resize the buffer */
                *dstbuffer = memRealloc(__FILE__, __LINE__, *dstbuffer,
                    offset, zh.total_out);
              }
            else
              {   /* we need a new buffer */
                *dstbuffer = memAlloc(__FILE__, __LINE__, zh.total_out);
              }

            /* Flawfinder: ignore */
            memcpy(*dstbuffer + offset, zbuffer, zh.total_out - offset);
            offset = zh.total_out;
          }

        if (error == Z_STREAM_END)
          { break; }

        if (error != Z_OK)
          {
            *errormsg = zh.msg;
            memFree(__FILE__, __LINE__, zbuffer, BUFFERSIZE);
            return 1;
          }
      }

    *dstlen = offset;

    memFree(__FILE__, __LINE__, zbuffer, BUFFERSIZE);

    error = inflateEnd(&zh);
    if (error != Z_OK)
      {
        *errormsg = zh.msg;
        return 1;
      }

    return 0;
  }

#undef BUFFERSIZE


/* #############################################################################
 */

