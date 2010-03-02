/* #############################################################################
 * header information for gpg.c
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
#ifndef CPM_GPG_H
#define CPM_GPG_H

/* #############################################################################
 * includes
 */
#include <assert.h>
#include <gpgme.h>


/* #############################################################################
 * global variables
 */
#define GPG_VERSION_MINIMUM "1.0.2"
typedef const char* (*PASSPHRASE_FN) (int retry, char* realm);


/* #############################################################################
 * prototypes
 */
void freeGPG(void);
void initGPG(void);
int gpgDecrypt(char* buffer, int size, char** newbuffer, int* newsize,
    PASSPHRASE_FN password_cb, SHOWERROR_FN showerror_cb);
int gpgEncrypt(char* buffer, int size, char** newbuffer, int* newsize,
    PASSPHRASE_FN password_cb, SHOWERROR_FN showerror_cb);
int gpgIsSecretKey(char* keyname);
char* gpgValidateEncryptionKey(char* keyname);


#endif


/* #############################################################################
 */

