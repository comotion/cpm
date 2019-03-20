/* #############################################################################
 * code for all the cryptography, handled via the GPGME library
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
#include "gpg.h"
#include "interface_keys.h"
#include "interface_utf8.h"
#include "listhandler.h"
#include "memory.h"
#include "string.h"


/* #############################################################################
 * internal functions
 */
#define gpgError(error)    fprintf(stderr, "GpgMe error (line %d): %s (%d)\n", \
                                __LINE__, gpgme_strerror(error), error)
int gpgCheckSignResult(SHOWERROR_FN showerror_cb, gpgme_sign_result_t result,
    gpgme_sig_mode_t type);
int gpgCheckVerifyResult(SHOWERROR_FN showerror_cb,
    gpgme_verify_result_t result, gpgme_error_t status);
char* gpgData2Char(gpgme_data_t dh, int* newsize);
#ifdef TEST_OPTION
void gpgDebugKey(gpgme_key_t key);
#endif
char* gpgGetFingerprint(char* keyname, int secret_only);
char* gpgGetRealm(const char* desc);
gpgme_error_t gpgRequestPassphrase(void *hook, const char *uid_hint,
    const char *passphrase_info, int last_was_bad, int fd);
#ifdef GPGME_HAS_RECIPIENT
int gpgGetRecipients(gpgme_recipient_t recipients,
    SHOWERROR_FN showerror_cb);
#endif


/* #############################################################################
 * global variables
 */
PASSPHRASE_FN           passphrase_callback;
int                     retries;
int                     signers;
char*                   lastrealm = NULL;

#define LIST_ALL        0
#define LIST_SECRET     1


/* #############################################################################
 *
 * Description    free the GPGME stuff
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      void
 * Return         void
 */
void freeGPG(void)
  {
    TRACE(99, "freeGPG()", NULL);

    if (lastrealm)
      {
        memFreeString(__FILE__, __LINE__, lastrealm);
        lastrealm = NULL;
      }
  }


/* #############################################################################
 *
 * Description    check the sign result and create the signing error messages
 *                accordingly
 * Author         Harry Brueckner
 * Date           2005-04-07
 * Arguments      SHOWERROR_FN                - error dialog to use
 *                gpgme_sign_result_t result  - GpgMe sign result
 *                gpgme_sig_mode_t type       - expected type
 * Return         0 if ok, 1 on error
 */
int gpgCheckSignResult(SHOWERROR_FN showerror_cb, gpgme_sign_result_t result,
    gpgme_sig_mode_t type)
  {
    gpgme_new_signature_t   signature;
    int                 error = 0,
                        counter = 0;
    char*               buffer;
    char*               hashname;

    TRACE(99, "gpgCheckSignResult()", NULL);

    buffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);

    if (!result)
      {
        snprintf(buffer, STDBUFFERLENGTH, _("signature could not be created"));
        error = 1;
      }
    if (!error &&
        result -> invalid_signers)
      {
        snprintf(buffer, STDBUFFERLENGTH, _("Invalid signer found: %s"),
            result -> invalid_signers -> fpr);
        error = 1;
      }
    if (!error)
      {   /* first we must count the signatures and compare them with the
           * number of keys we used
           */
        signature = result -> signatures;
        while (signature)
          {
            signature = signature -> next;
            counter++;
          }

        if (signers == 0)
          {   /* when we decrypt, we don't know the exact number */
            if (counter == 0)
              { error = 1; }
          }
        else
          {   /* during encryption we know the exact number we expect */
            if (counter != signers)
              { error = 1; }
          }

        if (error == 1)
          {   /* create the error message */
            snprintf(buffer, STDBUFFERLENGTH,
                _("Unexpected number of signatures created"));
          }
      }
    if (!error &&
        result -> signatures -> type != type)
      {
        snprintf(buffer, STDBUFFERLENGTH, _("Wrong type of signature created"));
        error = 1;
      }
    if (!error &&
        (result -> signatures -> pubkey_algo != GPGME_PK_DSA&&
         result -> signatures -> pubkey_algo != GPGME_PK_RSA))
      {
        snprintf(buffer, STDBUFFERLENGTH,
            _("Wrong pubkey algorithm reported: %i"),
            result -> signatures -> pubkey_algo);
        error = 1;
      }
    if (!error &&
        result -> signatures -> hash_algo != GPGME_MD_SHA1 &&
        result -> signatures -> hash_algo != GPGME_MD_SHA256 &&
        result -> signatures -> hash_algo != GPGME_MD_SHA384 &&
        result -> signatures -> hash_algo != GPGME_MD_SHA512
        )
      {
        switch (result -> signatures -> hash_algo)
          {
            case GPGME_MD_NONE:
                hashname = "none";
                break;
            case GPGME_MD_MD5:
                hashname = "md5";
                break;
            case GPGME_MD_SHA1:
                hashname = "sha1";
                break;
            case GPGME_MD_RMD160:
                hashname = "rmd160";
                break;
            case GPGME_MD_MD2:
                hashname = "md2";
                break;
            case GPGME_MD_TIGER:
                hashname = "tiger";
                break;
            case GPGME_MD_HAVAL:
                hashname = "haval";
                break;
            case GPGME_MD_SHA256:
                hashname = "sha256";
                break;
            case GPGME_MD_SHA384:
                hashname = "sha384";
                break;
            case GPGME_MD_SHA512:
                hashname = "sha512";
                break;
            case GPGME_MD_MD4:
                hashname = "md4";
                break;
            case GPGME_MD_CRC32:
                hashname = "crc32";
                break;
            case GPGME_MD_CRC32_RFC1510:
                hashname = "crc32 rfc1510";
                break;
            case GPGME_MD_CRC24_RFC2440:
                hashname = "crc24 rfc2440";
                break;
            default:
                hashname = NULL;
          }

        snprintf(buffer, STDBUFFERLENGTH,
            _("Wrong hash algorithm reported: %i (%s)"),
            result -> signatures -> hash_algo, hashname);
        error = 1;
      }
    if (!error &&
        result -> signatures -> sig_class != 0)
      {
        snprintf(buffer, STDBUFFERLENGTH,
            _("Wrong signature class reported: %u"),
            result -> signatures -> sig_class);
        error = 1;
      }

    if (error)
      { showerror_cb(_("GpgMe sign error"), buffer); }

    memFree(__FILE__, __LINE__, buffer, STDBUFFERLENGTH);

    return error;
  }


/* #############################################################################
 *
 * Description    check the verification result and create the signing error
 *                messages accordingly
 * Author         Harry Brueckner
 * Date           2005-04-08
 * Arguments      SHOWERROR_FN                  - error dialog to use
 *                gpgme_verify_result_t result  - GpgMe sign result
 *                gpgme_error_t status          - the status to check
 * Return         0 if ok, 1 on error
 */
int gpgCheckVerifyResult(SHOWERROR_FN showerror_cb,
    gpgme_verify_result_t result, gpgme_error_t status)
  {
    gpgme_signature_t   signature = NULL;
    int                 error = 0;
    char*               buffer;
    char*               validity;

    TRACE(99, "gpgCheckVerifyResult()", NULL);

    buffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);

    if (!result)
      {
        snprintf(buffer, STDBUFFERLENGTH, _("no signature result found"));
        error = 1;
      }
    else
      { signature = result -> signatures; }
    if (!error &&
        !signature)
      {   /* we don't check for exactly one signature - there might be more;
           * but there must be at least one valid signature
           */
        snprintf(buffer, STDBUFFERLENGTH, _("Database is not signed. Will not proceed!"));
        error = 1;
      }
    if (!error &&
       !signature->summary) {
       snprintf(buffer, STDBUFFERLENGTH, _("Database signed with keys of unknown validity. You must trust the key with fingerprint %s before proceeding"), signature->fpr);
       error = 1;
    } else if (!error &&
        (!(signature -> summary & GPGME_SIGSUM_VALID) ||
         !(signature -> summary & GPGME_SIGSUM_GREEN)))
      {
        if(signature -> summary & GPGME_SIGSUM_KEY_EXPIRED) {
           snprintf(buffer, STDBUFFERLENGTH, _("Signature valid but key %s has expired."), signature->fpr);
        } else if(signature -> summary & GPGME_SIGSUM_KEY_REVOKED) {
           snprintf(buffer, STDBUFFERLENGTH, _("Signature valid but key %s been revoked."), signature->fpr);
        } else if(signature -> summary & GPGME_SIGSUM_SIG_EXPIRED) {
           snprintf(buffer, STDBUFFERLENGTH, _("Database signature has expired for key %s."), signature->fpr);
        } else if(signature -> summary & GPGME_SIGSUM_KEY_MISSING) {
           snprintf(buffer, STDBUFFERLENGTH, _("Can't verify: key %s missing."), signature->fpr);
        } else if(signature -> summary & GPGME_SIGSUM_CRL_MISSING) {
           snprintf(buffer, STDBUFFERLENGTH, _("Certificate revocation list missing for key %s."), signature->fpr);
        } else if(signature -> summary & GPGME_SIGSUM_CRL_TOO_OLD) {
           snprintf(buffer, STDBUFFERLENGTH, _("Certificate revocation list too old for key %s."), signature->fpr);
        } else if(signature -> summary & GPGME_SIGSUM_BAD_POLICY) {
           snprintf(buffer, STDBUFFERLENGTH, _("A key or signature policy was not met for key %s."), signature->fpr);
        } else if(signature -> summary & GPGME_SIGSUM_SYS_ERROR) {
           snprintf(buffer, STDBUFFERLENGTH, _("A system error occurred for key %s."), signature->fpr);
        } else if(signature -> summary & GPGME_SIGSUM_RED) {
           snprintf(buffer, STDBUFFERLENGTH, _("Key signature %s is bad! Can not trust database."), signature->fpr);
        } else if(signature -> summary & GPGME_SIGSUM_GREEN) {
           snprintf(buffer, STDBUFFERLENGTH, _("Signature valid but key validity is 0x%x for key %s."),
            signature->validity, signature->fpr);
        }else{
           snprintf(buffer, STDBUFFERLENGTH, _("Unexpected signature summary: 0x%x on key %s"),
            signature -> summary, signature->fpr);
        }
        error = 1;
      }
    if (!error &&
        gpg_err_code(signature -> status) != status)
      {
        snprintf(buffer, STDBUFFERLENGTH, _("Unexpected signature status: %s on key %s"),
            gpgme_strerror(signature -> status), signature->fpr);
        error = 1;
      }
    /* notation data is not an error */
    /*
    if (!error &&
        signature -> notations)
      {
        snprintf(buffer, STDBUFFERLENGTH, _("Unexpected notation data on key %s:\n %s: %s"), 
                 signature->fpr, signature->notations->name, signature->notations->value);
        error = 1;
      }
      */
    if (!error &&
        signature -> wrong_key_usage)
      {
        snprintf(buffer, STDBUFFERLENGTH, _("Unexpectedly wrong key usage for key %s"), signature->fpr);
        error = 1;
      }
    if (!error &&
        signature -> validity != GPGME_VALIDITY_FULL)
      {
        switch (signature -> validity)
          {
            case GPGME_VALIDITY_UNKNOWN:
                validity = "unknown";
                break;
            case GPGME_VALIDITY_UNDEFINED:
                validity = "undefined";
                break;
            case GPGME_VALIDITY_NEVER:
                validity = "never";
                break;
            case GPGME_VALIDITY_MARGINAL:
                validity = "marginal";
                break;
            case GPGME_VALIDITY_FULL:
                validity = "full";
                break;
            case GPGME_VALIDITY_ULTIMATE:
                validity = "ultimate";
                break;
            default:
                validity = NULL;
                break;
          }
        snprintf(buffer, STDBUFFERLENGTH, _("Unexpected validity: %i (%s) on key %s"),
            signature -> validity, validity, signature->fpr);
        error = 1;
      }
    if (!error &&
        gpg_err_code (signature -> validity_reason) != GPG_ERR_NO_ERROR)
      {
        snprintf(buffer, STDBUFFERLENGTH, _("Unexpected validity reason: %s on key %s"),
            gpgme_strerror(signature -> validity_reason), signature->fpr);
        error = 1;
      }

    if (error)
      { showerror_cb(_("GpgMe verify error"), buffer); }

    memFree(__FILE__, __LINE__, buffer, STDBUFFERLENGTH);

    return error;
  }


/* #############################################################################
 *
 * Description    function to convert a GpgMe buffer into a char* buffer
 * Author         Harry Brueckner
 * Date           2005-04-01
 * Arguments      gpgme_data_t dh - the data handler
 *                int* newsize    - size of the returned buffer
 * Return         char* NULL on error, otherwise the encrypted data
 */
#define BUFFERSIZE      10240
char* gpgData2Char(gpgme_data_t dh, int* newsize)
  {
    size_t              tmpsize;
    char*               newbuffer;
    char*               tmpbuffer;

    TRACE(99, "gpgData2Char()", NULL);

    newbuffer = NULL;
    *newsize = 0;

    /* we have to rewind the buffer */
    if (gpgme_data_seek(dh, 0, SEEK_SET))
      {
        gpgError(gpgme_err_code_from_errno(errno));
        return NULL;
      }
    tmpbuffer = memAlloc(__FILE__, __LINE__, BUFFERSIZE + 1);
    while ((tmpsize = gpgme_data_read(dh, tmpbuffer, BUFFERSIZE)) > 0)
      {
        newbuffer = memRealloc(__FILE__, __LINE__, newbuffer,
            *newsize, *newsize + tmpsize);

        /* Flawfinder: ignore */
        memcpy(newbuffer + *newsize, tmpbuffer, tmpsize);
        *newsize += tmpsize;
      }
    memFree(__FILE__, __LINE__, tmpbuffer, BUFFERSIZE + 1);
    if (tmpsize < 0)
      {
        gpgError(gpgme_err_code_from_errno(errno));
        return NULL;
      }

    return newbuffer;
  }
#undef BUFFERSIZE


/* #############################################################################
 *
 * Description    display all given key data to stderr
 * Author         Harry Brueckner
 * Date           2005-09-18 19:11
 * Arguments      gpgme_key_t key     - the key to display
 * Return         void
 */
#ifdef TEST_OPTION
void gpgDebugKey(gpgme_key_t key)
  {
    TRACE(99, "gpgDebugKey()", NULL);

    gpgme_subkey_t      skey;
    gpgme_user_id_t     uid;

    fprintf(stderr, "=================================================\n");
    fprintf(stderr, "revoked:           %s\n", (key -> revoked) ? "yes" : "no");
    fprintf(stderr, "expired:           %s\n", (key -> expired) ? "yes" : "no");
    fprintf(stderr, "disabled:          %s\n", (key -> disabled) ? "yes" : "no");
    fprintf(stderr, "invalid:           %s\n", (key -> invalid) ? "yes" : "no");
    fprintf(stderr, "can_encrypt:       %s\n", (key -> can_encrypt) ? "yes" : "no");
    fprintf(stderr, "can_sign:          %s\n", (key -> can_sign) ? "yes" : "no");
    fprintf(stderr, "can_certify:       %s\n", (key -> can_certify) ? "yes" : "no");
    fprintf(stderr, "can_authenticate:  %s\n", (key -> can_authenticate) ? "yes" : "no");
    fprintf(stderr, "secret:            %s\n", (key -> secret) ? "yes" : "no");
    fprintf(stderr, "issuer name:      '%s'\n", key -> issuer_name);
    fprintf(stderr, "chain id:         '%s'\n", key -> chain_id);
    switch (key -> owner_trust)
      {
        case GPGME_VALIDITY_UNKNOWN:
            fprintf(stderr, "owner trust:       unknown\n");
            break;
        case GPGME_VALIDITY_UNDEFINED:
            fprintf(stderr, "owner trust:       undefined\n");
            break;
        case GPGME_VALIDITY_NEVER:
            fprintf(stderr, "owner trust:       never\n");
            break;
        case GPGME_VALIDITY_MARGINAL:
            fprintf(stderr, "owner trust:       marginal\n");
            break;
        case GPGME_VALIDITY_FULL:
            fprintf(stderr, "owner trust:       full\n");
            break;
        case GPGME_VALIDITY_ULTIMATE:
            fprintf(stderr, "owner trust:       ultimate\n");
            break;
        default:
            fprintf(stderr, "owner trust:       ERROR\n");
            break;
      }
    fprintf(stderr, "\n");

    skey = key -> subkeys;
    while (skey)
      {
        fprintf(stderr, "subkeys\n");
        fprintf(stderr, "\trevoked:           %s\n", (skey -> revoked) ? "yes" : "no");
        fprintf(stderr, "\texpired:           %s\n", (skey -> expired) ? "yes" : "no");
        fprintf(stderr, "\tdisabled:          %s\n", (skey -> disabled) ? "yes" : "no");
        fprintf(stderr, "\tinvalid:           %s\n", (skey -> invalid) ? "yes" : "no");
        fprintf(stderr, "\tcan_encrypt:       %s\n", (skey -> can_encrypt) ? "yes" : "no");
        fprintf(stderr, "\tcan_sign:          %s\n", (skey -> can_sign) ? "yes" : "no");
        fprintf(stderr, "\tcan_certify:       %s\n", (skey -> can_certify) ? "yes" : "no");
        fprintf(stderr, "\tcan_authenticate:  %s\n", (skey -> can_authenticate) ? "yes" : "no");
        fprintf(stderr, "\tsecret:            %s\n", (skey -> secret) ? "yes" : "no");
        fprintf(stderr, "\tpubkey algo:      '%s'\n", gpgme_pubkey_algo_name(skey -> pubkey_algo));
        fprintf(stderr, "\tlength:            %d\n", skey -> length);
        fprintf(stderr, "\tkey id:           '%s'\n", skey -> keyid);
        fprintf(stderr, "\tfpr:              '%s'\n", skey -> fpr);
        fprintf(stderr, "\ttimestamp:         %ld\n", skey -> timestamp);
        fprintf(stderr, "\texpires:           %ld\n", skey -> expires);
        skey = skey -> next;
      }
    fprintf(stderr, "\n");

    uid = key -> uids;
    while (uid)
      {
        fprintf(stderr, "uid\n");
        fprintf(stderr, "\trevoked:           %s\n", (uid -> revoked) ? "yes" : "no");
        fprintf(stderr, "\tinvalid:           %s\n", (uid -> invalid) ? "yes" : "no");
        switch (uid -> validity)
          {
            case GPGME_VALIDITY_UNKNOWN:
                fprintf(stderr, "\towner trust:       unknown\n");
                break;
            case GPGME_VALIDITY_UNDEFINED:
                fprintf(stderr, "\towner trust:       undefined\n");
                break;
            case GPGME_VALIDITY_NEVER:
                fprintf(stderr, "\towner trust:       never\n");
                break;
            case GPGME_VALIDITY_MARGINAL:
                fprintf(stderr, "\towner trust:       marginal\n");
                break;
            case GPGME_VALIDITY_FULL:
                fprintf(stderr, "\towner trust:       full\n");
                break;
            case GPGME_VALIDITY_ULTIMATE:
                fprintf(stderr, "\towner trust:       ultimate\n");
                break;
            default:
                fprintf(stderr, "\towner trust:       ERROR\n");
                break;
          }
        fprintf(stderr, "\tuid:              '%s'\n", uid -> uid);
        fprintf(stderr, "\tname:             '%s'\n", uid -> name);
        fprintf(stderr, "\tcomment:          '%s'\n", uid -> comment);
        fprintf(stderr, "\temail:            '%s'\n", uid -> email);
        uid = uid -> next;
      }
  }
#endif


/* #############################################################################
 *
 * Description    encrypt the given buffer and return the encrypted data with
 *                an updated size information
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      char* buffer      - buffer to encrypt
 *                int size          - size of the buffer
 *                char** newbuffer  - pointer to the new buffer which holds the
 *                                    encrypted data
 *                int* newsize      - size of the returned buffer
 *                PASSPHRASE_FN password_cb   - callback function pointer used
 *                                              to get the current passphrase
 *                SHOWERROR_FN showerror_cb   - callback function pointer used
 *                                              to display errors
 * Return         0 if ok, otherwise 1
 */
int gpgDecrypt(char* buffer, int size, char** newbuffer, int* newsize,
    PASSPHRASE_FN password_cb, SHOWERROR_FN showerror_cb)
  {
    gpgme_ctx_t         context;
    gpgme_error_t       error;
    gpgme_data_t        input,
                        output;
    gpgme_decrypt_result_t  decrypt_result = NULL;
    gpgme_verify_result_t   verify_result;
    int                 showerror = 1;
    char*               agent;
    char*               tmpbuffer = NULL;

    TRACE(99, "gpgDecrypt()", NULL);

    /* we set our passphrase callback function */
    passphrase_callback = password_cb;

    *newbuffer = NULL;
    *newsize = 0;

    error = gpgme_new(&context);
    if (error)
      {
        (showerror_cb)(_("GpgMe context error"), gpgme_strerror(error));
        return 1;
      }
    else
      {   /* we got a context, we set the passphrase callback */
        /* Flawfinder: ignore */
        agent = getenv("GPG_AGENT_INFO");
        if (!(agent && strchr(agent, ':')))
          {
            retries = 0;
            gpgme_set_passphrase_cb(context, gpgRequestPassphrase, NULL);
          }
      }

    if (!error)
      { error = gpgme_data_new_from_mem(&input, buffer, size, 0); }

    if (!error)
      { error = gpgme_data_new(&output); }

    if (!error)
      { error = gpgme_op_decrypt_verify(context, input, output); }

    if (!error)
      { decrypt_result = gpgme_op_decrypt_result(context); }

    if (!error &&
        decrypt_result &&
        decrypt_result -> unsupported_algorithm)
      {
        tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
        snprintf(tmpbuffer, STDBUFFERLENGTH, _("unsupported algorithm: %s\n"),
            decrypt_result -> unsupported_algorithm);
        (showerror_cb)(_("GpgMe error"), tmpbuffer);
        memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

        showerror = 0;
        error = 1;
      }

    if (!error)
      {
        verify_result = gpgme_op_verify_result(context);
        error = gpgCheckVerifyResult(showerror_cb, verify_result,
            GPG_ERR_NO_ERROR);
        showerror = !error;
      }

    /* we don't need the passphrase any longer */
    clearPassphrase(0);

    if (!error)
      { tmpbuffer = gpgData2Char(output, newsize); }

#ifdef GPGME_HAS_RECIPIENT
    /* we get the recipients of the message for the further re-encryption of
     * the file
     */
    if (!error &&
        decrypt_result &&
        decrypt_result -> recipients)
      {
        error = gpgGetRecipients(decrypt_result -> recipients, showerror_cb);
        showerror = 0;
      }
#endif

    gpgme_data_release(input);
    gpgme_data_release(output);
    gpgme_release(context);

    *newbuffer = tmpbuffer;

    if (error)
      {
        if (showerror)
          { (showerror_cb)(_("GpgMe decrypt error"), gpgme_strerror(error)); }
        return 1;
      }
    else
        return 0;
  }


/* #############################################################################
 *
 * Description    encrypt the given buffer and return the encrypted data with
 *                an updated size information
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      char* buffer      - buffer to encrypt
 *                int size          - size of the buffer
 *                char** newbuffer  - pointer to the new buffer which holds the
 *                                    encrypted data
 *                int* newsize      - size of the returned buffer
 *                PASSPHRASE_FN password_cb   - callback function pointer used
 *                                              to get the current passphrase
 *                SHOWERROR_FN showerror_cb   - callback function pointer used
 *                                              to display errors
 * Return         0 if ok, otherwise 1
 */
int gpgEncrypt(char* buffer, int size, char** newbuffer, int* newsize,
    PASSPHRASE_FN password_cb, SHOWERROR_FN showerror_cb)
  {
    gpgme_ctx_t         context;
    gpgme_data_t        input,
                        output;
    gpgme_encrypt_result_t  result;
    gpgme_error_t       error;
    gpgme_key_t*        key = NULL;
    gpgme_key_t         tkey = NULL;
    gpgme_sign_result_t sign_result;
    int                 i,
                        keys = 0,
                        showerror = 1;
    char*               agent;
    char*               fpr;
    char*               tmpbuffer = NULL;

    TRACE(99, "gpgEncrypt()", NULL);

    /* we set our passphrase callback function */
    passphrase_callback = password_cb;

    /* we initialize the external size data */
    newsize[0] = 0;

    error = gpgme_new(&context);

    if (!error)
      {
        gpgme_set_textmode(context, 1);
        gpgme_set_armor(context, 1);

        /* Flawfinder: ignore */
        agent = getenv("GPG_AGENT_INFO");
        if (!(agent && strchr(agent, ':')))
          {
            retries = 0;
            gpgme_set_passphrase_cb(context, gpgRequestPassphrase, NULL);
          }
      }

    if (!error)
      { error = gpgme_data_new_from_mem(&input, buffer, size, 0); }

    if (!error)
      { error = gpgme_data_new(&output); }

    if (!error)
      { gpgme_signers_clear(context); }

    if (!error)
      {
        /* allocate the keys */
        keys = keyCount();
        key = memAlloc(__FILE__, __LINE__, sizeof(gpgme_key_t) * (keys + 1));
        key[keys] = NULL;
        signers = 0;
        for (i = 0; i < keys && !error; i++)
          {   /* add all keys */
            fpr = gpgGetFingerprint(keyGet(i), LIST_SECRET);
            if (fpr)
              {
                error = gpgme_get_key(context, fpr, &tkey, LIST_SECRET);
                if (tkey -> secret)
                  {
                    error = gpgme_signers_add(context, tkey);
                    signers++;
                  }

                memFreeString(__FILE__, __LINE__, fpr);
              }

            fpr = gpgGetFingerprint(keyGet(i), LIST_ALL);
            if (fpr)
              {
                error = gpgme_get_key(context, fpr, &key[i], LIST_ALL);
                memFreeString(__FILE__, __LINE__, fpr);
              }
          }
      }

    if (signers > 1)
      {   /* as soon as we get two signers, we must no longer cache anything */
        config -> keeppassphrase = 0;
        clearPassphrase(1);
      }

    /* encrypt and sign the data */
    if (!error)
      {
        error = gpgme_op_encrypt_sign(context, key, GPGME_ENCRYPT_ALWAYS_TRUST,
            input, output);
      }

    /* we don't need the passphrase any longer */
    clearPassphrase(0);

    if (!error)
      { result = gpgme_op_encrypt_result(context); }
    if (!error &&
        result -> invalid_recipients)
      {
        tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
        snprintf(tmpbuffer, STDBUFFERLENGTH,
            _("Invalid recipient encountered: %s"),
            result -> invalid_recipients -> fpr);
        (showerror_cb)(_("GpgMe error"), tmpbuffer);
        memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);

        showerror = 0;
        error = 1;
      }

    if (!error)
      {
        sign_result = gpgme_op_sign_result(context);
        error = gpgCheckSignResult(showerror_cb, sign_result,
            GPGME_SIG_MODE_NORMAL);
        showerror = !error;
      }

    if (!error)
      { tmpbuffer = gpgData2Char(output, newsize); }

    /* free the keys again */
    i = 0;
    while (key && keys && key[i])
      { gpgme_key_unref(key[i++]); }
    memFree(__FILE__, __LINE__, key, sizeof(gpgme_key_t) * (keys + 1));

    gpgme_data_release(input);
    gpgme_data_release(output);
    gpgme_release(context);

    *newbuffer = tmpbuffer;

    if (error)
      {
        if (showerror)
          { (showerror_cb)(_("GpgMe encrypt error"), gpgme_strerror(error)); }
        return 1;
      }
    else
        return 0;
  }


/* #############################################################################
 *
 * Description    find a fingerprint for the given key
 * Author         Harry Brueckner
 * Date           2005-04-07
 * Arguments      keyname       - description string
 *                secret_only   - if set to 1, only secret keys are listed
 * Return         char* with the found fingerprint; the string must be freed
 *                by the caller
 */
char* gpgGetFingerprint(char* keyname, int secret_only)
  {
    gpgme_ctx_t         context;
    gpgme_key_t         key;
    gpgme_error_t       error;
    char*               identifier = NULL;

    TRACE(99, "gpgGetFingerprint()", NULL);

    if (!config -> encryptdata)
      { return NULL; }

    /* get a new context */
    error = gpgme_new(&context);
    if (error)
      {
        gpgme_release(context);
        gpgError(error);
        return NULL;
      }

    /* start cycling through the list of keys */
    error = gpgme_op_keylist_start(context, keyname, secret_only);
    if (error)
      {
        gpgme_release(context);
        gpgError(error);
        return NULL;
      }

    /* first we look for secret keys */
    while (!identifier &&
        !(error = gpgme_op_keylist_next(context, &key)))
      {   /* take the first key we find */
        if (!identifier &&
            !key -> disabled &&
            !key -> expired &&
            !key -> invalid &&
            !key -> revoked)
          {   /* we just use keys we can encrypt for */
            identifier = memAlloc(__FILE__, __LINE__,
                strlen(key -> subkeys -> fpr) + 1);
            strStrncpy(identifier, key -> subkeys -> fpr,
                strlen(key -> subkeys -> fpr) + 1);
          }

        gpgme_key_unref(key);
      }

    if (error &&
        gpg_err_code(error) != GPG_ERR_EOF)
      {    /* we validate the last value of the 'next' operation */
        gpgme_release(context);
        gpgError(error);
        return NULL;
      }

    /* finish the key listing */
    error = gpgme_op_keylist_end(context);
    if (error)
      {
        gpgme_release(context);
        gpgError(error);
        return NULL;
      }

    gpgme_release(context);

    return identifier;
  }


/* #############################################################################
 *
 * Description    extract just the name of the realm from the description
 *                of the passphrase callback function
 * Author         Harry Brueckner
 * Date           2005-04-04
 * Arguments      const char* desc  - description string
 * Return         char* containing the realm string; the string must be freed
 *                by the caller
 */
char* gpgGetRealm(const char* desc)
  {
    char*               p = (char*)desc;
    char*               realm;
    char*               start;
    int                 size = 0;

    TRACE(99, "gpgGetRealm()", NULL);

    if (!desc)
      { return NULL; }

    /* we skip the key id */
    while (p[0] && p[0] != ' ')
      { p++; }

    start = ++p;

    /* and see how long our string is */
    while (p[0] && p[0] != '\n')
      { p++; size++; }

    realm = memAlloc(__FILE__, __LINE__, size + 1);
    realm[size] = 0;
    strncpy(realm, start, size);

    return realm;
  }


/* #############################################################################
 * Description    get the recipients of the current context and add them to the
 *                list of our recipients
 * Author         Harry Brueckner
 * Date           2006-02-19
 * Arguments      gpgme_ctx_t context   - context to get the recipients from
 *                SHOWERROR_FN showerror_cb   - callback function pointer used
 *                                              to display errors
 * Return         0 if ok, otherwise 1
 */
#ifdef GPGME_HAS_RECIPIENT
int gpgGetRecipients(gpgme_recipient_t recipients,
    SHOWERROR_FN showerror_cb)
  {
    gpgme_recipient_t   recipient = recipients;
    char*               keyname = NULL;
    char*               tmpbuffer = NULL;

    TRACE(99, "gpgGetRecipients()", NULL);

    while (recipient)
      {
        /* if the keyid is NULL, we skip this key */
        if (recipient -> keyid == NULL)
          { continue; }

        keyname = gpgValidateEncryptionKey(recipient -> keyid);

        if (keyname)
          {   /* add the key to the global list of keys */
            config -> defaultkeys = listAdd(config -> defaultkeys,
                keyname);
            memFreeString(__FILE__, __LINE__, keyname);
          }
        else
          {   /* report an error, the key can not be found */
            tmpbuffer = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
            snprintf(tmpbuffer, STDBUFFERLENGTH,
                _("unknown recipient id %s"), recipient -> keyid);
            (showerror_cb)(_("GpgMe recipient error"), tmpbuffer);
            memFree(__FILE__, __LINE__, tmpbuffer, STDBUFFERLENGTH);
            /* user shouldn't write db when she is missing keys */
            runtime->readonly = 1;
          }

        recipient = recipient -> next;
      }

    return 0;
  }
#endif


/* #############################################################################
 *
 * Description    request the users passphrase for the de-/encryption process
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      void* hook                    - unknown (gpgme internal)
 *                const char *uid_hint          - user id the passphrase is
 *                                                required for
 *                const char *passphrase_info   - additional, operation
 *                                                dependend user information
 *                int last_was_bad              - 1 if the last entered
 *                                                passphrase was bad, otherwise
 *                                                0
 *                int fd                        - file descriptor where the
 *                                                passphrase should come from
 * Return         gpgme_error_t error status
 */
gpgme_error_t gpgRequestPassphrase(void *hook, const char *uid_hint,
    const char *passphrase_info, int last_was_bad, int fd)
  {
    ssize_t             len,
                        wsize;
    char*               ptr;

    TRACE(99, "gpgRequestPassphrase()", NULL);

    if (!uid_hint)
      { return GPG_ERR_GENERAL; }

    if (last_was_bad && retries < 3)
      {   /* the last passphrase was bad */
        clearPassphrase(1);
      }

    if (runtime -> realm)
      { memFreeString(__FILE__, __LINE__, runtime -> realm); }
    runtime -> realm = gpgGetRealm(uid_hint);

    if (lastrealm && strcmp(lastrealm, runtime -> realm))
      {   /* we got a new realm so we must reset our password right away
           * and the same goes for the retry counter
           */
        clearPassphrase(1);
        retries = 0;
      }

    if (lastrealm)
      { memFreeString(__FILE__, __LINE__, lastrealm); }
    lastrealm = memAlloc(__FILE__, __LINE__, strlen(runtime -> realm) + 1);
    strStrncpy(lastrealm, runtime -> realm, strlen(runtime -> realm) + 1);

    /* we keep the hint for the key addition */
    if (runtime -> realmhint)
      { memFreeString(__FILE__, __LINE__, runtime -> realmhint); }
    runtime -> realmhint = memAlloc(__FILE__, __LINE__, strlen(uid_hint) + 1);
    strStrncpy(runtime -> realmhint, uid_hint, strlen(uid_hint) + 1);

#ifdef TEST_OPTION
    if (config -> testrun)
      {   /* if we run in testmode, we have a static passphrase */
        strStrncpy(runtime -> passphrase, "1234567890", 10 + 1);
      }
    else
      { (passphrase_callback)(++retries, runtime -> realm); }
#else
    (passphrase_callback)(++retries, runtime -> realm);
#endif

    ptr = memAlloc(__FILE__, __LINE__, strlen(runtime -> passphrase) + 2);
    snprintf(ptr, strlen(runtime -> passphrase) + 2, "%s\n",
        runtime -> passphrase);
    len = strlen(ptr);
    wsize = write(fd, ptr, len);
    memFreeString(__FILE__, __LINE__, ptr);

    if (wsize == len)
      { return GPG_ERR_NO_ERROR; }
    else
      { return GPG_ERR_GENERAL; }
  }


/* #############################################################################
 *
 * Description    check if for the given keyname a secret key exists
 * Author         Harry Brueckner
 * Date           2005-04-25
 * Arguments      char* keyname   - the key to check
 * Return         int 1 if there is a secret key, 0 if not and -1 if a gpg
 *                      error occured
 */
int gpgIsSecretKey(char* keyname)
  {
    gpgme_ctx_t         context;
    gpgme_key_t         key;
    gpgme_error_t       error;
    int                 secret = 0;

    TRACE(99, "gpgIsSecretKey()", NULL);

    if (!config -> encryptdata)
      { return 0; }

    /* get a new context */
    error = gpgme_new(&context);
    if (error)
      {
        gpgme_release(context);
        gpgError(error);
        return -1;
      }

    /* start cycling through the list of keys */
    error = gpgme_op_keylist_start(context, keyname, LIST_SECRET);
    if (error)
      {
        gpgme_release(context);
        gpgError(error);
        return -1;
      }

    while (!(error = gpgme_op_keylist_next(context, &key)))
      {   /* take the first usable key we find */
        /* TODO: only choose usable secret keys */
        if (key -> can_encrypt &&
            key -> secret &&
            !key -> disabled &&
            !key -> expired &&
            !key -> invalid &&
            !key -> revoked)
          {   /* we just use keys we can encrypt for */
            secret = 1;
          }

        gpgme_key_unref(key);

        if (secret)
          { break; }
      }

    if (error &&
        gpg_err_code(error) != GPG_ERR_EOF)
      {    /* we validate the last value of the 'next' operation */
        gpgme_release(context);
        gpgError(error);
        return -1;
      }

    /* finish the key listing */
    error = gpgme_op_keylist_end(context);
    if (error)
      {
        gpgme_release(context);
        gpgError(error);
        return -1;
      }

    gpgme_release(context);

    return secret;
  }


/* #############################################################################
 *
 * Description    validate the given encryption key
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      char* key   - the key to validate
 * Return         char* NULL on error, otherwise the name and mail address
 */
char* gpgValidateEncryptionKey(char* keyname)
  {
    gpgme_ctx_t         context;
    gpgme_key_t         key;
    gpgme_error_t       error;
    int                 secret,
                        size;
    char*               identifier = NULL;
    char*               tcomment;
    char*               tname;

    TRACE(99, "gpgValidateEncryptionKey()", NULL);

    if (!config -> encryptdata)
      { return NULL; }

    /* get a new context */
    error = gpgme_new(&context);
    if (error)
      {
        gpgme_release(context);
        gpgError(error);
        return NULL;
      }

    for (secret = 1; secret >= 0 && !identifier; secret--)
      {
        /* start cycling through the list of keys */
        error = gpgme_op_keylist_start(context, keyname,
            (secret == 1) ? LIST_SECRET : LIST_ALL);
        if (error)
          {
            gpgme_release(context);
            gpgError(error);
            return NULL;
          }

        while (!(error = gpgme_op_keylist_next(context, &key)))
          {   /* take the first key we find */
#ifdef TEST_OPTION
  #ifdef KEY_DEBUG
            gpgDebugKey(key);
  #endif
#endif
            if (key -> can_encrypt &&
                !key -> disabled &&
                !key -> expired &&
                !key -> invalid &&
                !key -> revoked)
              {   /* we just use keys we can encrypt for and sign with */
                tname = convert2terminal((unsigned char*)key -> uids -> name);
                if (key -> uids -> comment)
                  { tcomment = key -> uids -> comment; }
                else
                  { tcomment = NULL; }

                if (tcomment && strlen(tcomment))
                  {   /* a comment exists for this key */
                    size = strlen(key -> subkeys -> keyid) + 1 +
                        strlen(tname) + 1 +
                        strlen(tcomment) + 2 + 1 +
                        strlen(key -> uids -> email) + 2 + 1;
                    identifier = memAlloc(__FILE__, __LINE__, size);
                    snprintf(identifier, size, "%s %s (%s) <%s>",
                        key -> subkeys -> keyid,
                        tname,
                        tcomment,
                        key -> uids -> email);
                  }
                else
                  {   /* no comment exists */
                    size = strlen(key -> subkeys -> keyid) + 1 +
                        strlen(tname) + 1 +
                        strlen(key -> uids -> email) + 2 + 1;
                    identifier = memAlloc(__FILE__, __LINE__, size);
                    snprintf(identifier, size, "%s %s <%s>",
                        key -> subkeys -> keyid,
                        tname,
                        key -> uids -> email);
                  }
              }

            gpgme_key_unref(key);

            if (identifier)
              { break; }
          }

        if (error &&
            gpg_err_code(error) != GPG_ERR_EOF)
          {    /* we validate the last value of the 'next' operation */
            gpgme_release(context);
            gpgError(error);
            return NULL;
          }
      }

    /* finish the key listing */
    error = gpgme_op_keylist_end(context);
    if (error)
      {
        gpgme_release(context);
        gpgError(error);
        return NULL;
      }

    gpgme_release(context);

    return identifier;
  }


/* #############################################################################
 *
 * Description    initialize the GPGME library
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      void
 * Return         void
 */
void initGPG(void)
  {
    gpgme_error_t       error;

    gpgme_check_version(NULL);

    TRACE(99, "initGPG()", NULL);

    error = gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);
    if (error)
      {
        gpgError(error);
        exit(1);
      }

    if (!gpgme_check_version(GPG_VERSION_MINIMUM))
      {
        fprintf(stderr, _("GpgMe error: invalid library version (%s).\n"),
            gpgme_check_version(NULL));
        exit(1);
      }

    retries = 0;
    signers = 0;
    lastrealm = NULL;
  }


/* #############################################################################
 */

