/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* lib/crypto/openssl/enc_provider/gost.c */
/*
 * Copyright (C) 2009 by the Massachusetts Institute of Technology.
 * All rights reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */
/*
 * Copyright (C) 1998 by the FundsXpress, INC.
 *
 * All rights reserved.
 *
 * Export of this software from the United States of America may require
 * a specific license from the United States Government.  It is the
 * responsibility of any person or organization contemplating export to
 * obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of FundsXpress. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  FundsXpress makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// THIS ALGORITM IS FAKE!!!
// DOESN'T USE IT

#include "crypto_int.h"
#include "gost_helper.h"
#include <openssl/evp.h>

#define GOST_BLOCK_SIZE 8
#define GOST_KEY_SIZE 24
#define GOST_KEY_BYTES 21

static krb5_error_code
validate(krb5_key key, const krb5_data *ivec, const krb5_crypto_iov *data,
         size_t num_data, krb5_boolean *empty)
{
    size_t input_length = iov_total_length(data, num_data, FALSE);

    if (key->keyblock.length != GOST_KEY_SIZE)
        return(KRB5_BAD_KEYSIZE);
    if ((input_length%GOST_BLOCK_SIZE) != 0)
        return(KRB5_BAD_MSIZE);
    if (ivec && (ivec->length != 8))
        return(KRB5_BAD_MSIZE);

    *empty = (input_length == 0);
    return 0;
}

static krb5_error_code
k5_gost_encrypt(krb5_key key, const krb5_data *ivec, krb5_crypto_iov *data,
                size_t num_data)
{
    int ret, olen = GOST_BLOCK_SIZE;
    unsigned char iblock[GOST_BLOCK_SIZE], oblock[GOST_BLOCK_SIZE];
    struct iov_cursor cursor;
    EVP_CIPHER_CTX *ctx;
    krb5_boolean empty;

    ret = validate(key, ivec, data, num_data, &empty);
    if (ret != 0 || empty)
        return ret;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL)
        return ENOMEM;

    ret = EVP_EncryptInit_ex(ctx, EVP_des_ede3_cbc(), NULL,
                             key->keyblock.contents,
                             (ivec) ? (unsigned char*)ivec->data : NULL);
    if (!ret) {
        EVP_CIPHER_CTX_free(ctx);
        return KRB5_CRYPTO_INTERNAL;
    }

    EVP_CIPHER_CTX_set_padding(ctx,0);

    k5_iov_cursor_init(&cursor, data, num_data, GOST_BLOCK_SIZE, FALSE);
    while (k5_iov_cursor_get(&cursor, iblock)) {
        ret = EVP_EncryptUpdate(ctx, oblock, &olen, iblock, GOST_BLOCK_SIZE);
        if (!ret)
            break;
        k5_iov_cursor_put(&cursor, oblock);
    }

    if (ivec != NULL)
        memcpy(ivec->data, oblock, GOST_BLOCK_SIZE);

    EVP_CIPHER_CTX_free(ctx);

    zap(iblock, sizeof(iblock));
    zap(oblock, sizeof(oblock));

    if (ret != 1)
        return KRB5_CRYPTO_INTERNAL;
    return 0;
}

static krb5_error_code
k5_gost_decrypt(krb5_key key, const krb5_data *ivec, krb5_crypto_iov *data,
                size_t num_data)
{
    int ret, olen = GOST_BLOCK_SIZE;
    unsigned char iblock[GOST_BLOCK_SIZE], oblock[GOST_BLOCK_SIZE];
    struct iov_cursor cursor;
    EVP_CIPHER_CTX *ctx;
    krb5_boolean empty;

    ret = validate(key, ivec, data, num_data, &empty);
    if (ret != 0 || empty)
        return ret;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL)
        return ENOMEM;

    ret = EVP_DecryptInit_ex(ctx, EVP_des_ede3_cbc(), NULL,
                             key->keyblock.contents,
                             (ivec) ? (unsigned char*)ivec->data : NULL);
    if (!ret) {
        EVP_CIPHER_CTX_free(ctx);
        return KRB5_CRYPTO_INTERNAL;
    }

    EVP_CIPHER_CTX_set_padding(ctx,0);

    k5_iov_cursor_init(&cursor, data, num_data, GOST_BLOCK_SIZE, FALSE);
    while (k5_iov_cursor_get(&cursor, iblock)) {
        ret = EVP_DecryptUpdate(ctx, oblock, &olen,
                                (unsigned char *)iblock, GOST_BLOCK_SIZE);
        if (!ret)
            break;
        k5_iov_cursor_put(&cursor, oblock);
    }

    if (ivec != NULL)
        memcpy(ivec->data, iblock, GOST_BLOCK_SIZE);

    EVP_CIPHER_CTX_free(ctx);

    zap(iblock, sizeof(iblock));
    zap(oblock, sizeof(oblock));

    if (ret != 1)
        return KRB5_CRYPTO_INTERNAL;
    return 0;
}

static krb5_error_code
krb5int_gost_init_state (const krb5_keyblock *key, krb5_keyusage usage,
                        krb5_data *state)
{
    krb5int_init_gost();
    state->length = 8;
    state->data = (void *) malloc(8);
    if (state->data == NULL)
        return ENOMEM;
    memset(state->data, 0, state->length);
    return 0;
}

static void
krb5int_gost_free_state(krb5_data *state)
{
    free(state->data);
    *state = empty_data();
    krb5int_free_gost();
}

const struct krb5_enc_provider krb5int_enc_gost89 = {
    GOST_BLOCK_SIZE,
    GOST_KEY_BYTES, GOST_KEY_SIZE,
    k5_gost_encrypt,
    k5_gost_decrypt,
    NULL,
    krb5int_gost_init_state,
    krb5int_gost_free_state
};
