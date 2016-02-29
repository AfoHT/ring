/* ====================================================================
 * Copyright (c) 2008 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ==================================================================== */

#ifndef OPENSSL_HEADER_MODES_INTERNAL_H
#define OPENSSL_HEADER_MODES_INTERNAL_H

#include <openssl/aes.h>

#include "../internal.h"

#if defined(__cplusplus)
extern "C" {
#endif


/* aes_block_f is a pointer to |AES_Encrypt| or a variant thereof. */
typedef void (*aes_block_f)(const uint8_t in[16], uint8_t out[16],
                            const AES_KEY *key);


/* GCM definitions */
typedef struct { uint64_t hi,lo; } u128;

/* This differs from OpenSSL's |gcm128_context| in that it does not have the
 * |key| pointer, in order to make it |memcpy|-friendly. See openssl/modes.h
 * for more info. */
struct gcm128_context {
  /* Following 6 names follow names in GCM specification */
  union {
    uint64_t u[2];
    uint32_t d[4];
    uint8_t c[16];
  } Yi, EKi, EK0, len, Xi, H;

  /* Relative position of Xi, H and pre-computed Htable is used in some
   * assembler modules, i.e. don't change the order! */
  u128 Htable[16];
  void (*gmult)(uint64_t Xi[2], const u128 Htable[16]);
  void (*ghash)(uint64_t Xi[2], const u128 Htable[16], const uint8_t *inp,
                size_t len);

  aes_block_f block;
};

#if defined(OPENSSL_X86) || defined(OPENSSL_X86_64)
/* crypto_gcm_clmul_enabled returns one if the CLMUL implementation of GCM is
 * used. */
int crypto_gcm_clmul_enabled(void);
#endif


/* CTR. */

/* aes_ctr_f is the type of a function that performs CTR-mode encryption with
 * AES. */
typedef void (*aes_ctr_f)(const uint8_t *in, uint8_t *out, size_t blocks,
                          const AES_KEY *key, const uint8_t ivec[16]);

/* GCM.
 *
 * This API differs from the OpenSSL API slightly. The |GCM128_CONTEXT| does
 * not have a |key| pointer that points to the key as OpenSSL's version does.
 * Instead, every function takes a |key| parameter. This way |GCM128_CONTEXT|
 * can be safely copied. */

typedef struct gcm128_context GCM128_CONTEXT;

/* CRYPTO_gcm128_init initialises |ctx| to use |block| (typically AES) with
 * the given key. */
OPENSSL_EXPORT void CRYPTO_gcm128_init(GCM128_CONTEXT *ctx, const AES_KEY *key,
                                       aes_block_f block);

/* CRYPTO_gcm128_set_96_bit_iv sets the IV (nonce) for |ctx|. The |key| must be
 * the same key that was passed to |CRYPTO_gcm128_init|. |aad| is the
 * authenticated data and |aad_len| is its length. It returns one on success
 * and zero otherwise.*/
OPENSSL_EXPORT int CRYPTO_gcm128_message_init(GCM128_CONTEXT *ctx,
                                              const AES_KEY *key,
                                              const uint8_t iv[12],
                                              const uint8_t *aad,
                                              size_t aad_len);

/* CRYPTO_gcm128_encrypt encrypts |len| bytes from |in| to |out|. The |key|
 * must be the same key that was passed to |CRYPTO_gcm128_init|. It returns one
 * on success and zero otherwise. */
OPENSSL_EXPORT int CRYPTO_gcm128_encrypt_oneshot(GCM128_CONTEXT *ctx,
                                                 const AES_KEY *key,
                                                 const uint8_t *in,
                                                 uint8_t *out, size_t len);

/* CRYPTO_gcm128_decrypt decrypts |len| bytes from |in| to |out|. The |key|
 * must be the same key that was passed to |CRYPTO_gcm128_init|. It returns one
 * on success and zero otherwise. */
OPENSSL_EXPORT int CRYPTO_gcm128_decrypt_oneshot(GCM128_CONTEXT *ctx,
                                                 const AES_KEY *key,
                                                 const uint8_t *in,
                                                 uint8_t *out, size_t len);

/* CRYPTO_gcm128_encrypt_ctr32 encrypts |len| bytes from |in| to |out| using
 * a CTR function that only handles the bottom 32 bits of the nonce, like
 * |CRYPTO_ctr128_encrypt_ctr32|. The |key| must be the same key that was
 * passed to |CRYPTO_gcm128_init|. It returns one on success and zero
 * otherwise. */
OPENSSL_EXPORT int CRYPTO_gcm128_encrypt_ctr32_oneshot(GCM128_CONTEXT *ctx,
                                                       const AES_KEY *key,
                                                       const uint8_t *in,
                                                       uint8_t *out, size_t len,
                                                       aes_ctr_f stream);

/* CRYPTO_gcm128_decrypt_ctr32 decrypts |len| bytes from |in| to |out| using
 * a CTR function that only handles the bottom 32 bits of the nonce, like
 * |CRYPTO_ctr128_encrypt_ctr32|. The |key| must be the same key that was
 * passed to |CRYPTO_gcm128_init|. It returns one on success and zero
 * otherwise. */
OPENSSL_EXPORT int CRYPTO_gcm128_decrypt_ctr32_oneshot(GCM128_CONTEXT *ctx,
                                                       const AES_KEY *key,
                                                       const uint8_t *in,
                                                       uint8_t *out, size_t len,
                                                       aes_ctr_f stream);

/* CRYPTO_gcm128_tag calculates the authenticator and copies it into |tag|.
 * The minimum of |len| and 16 bytes are copied into |tag|. */
OPENSSL_EXPORT void CRYPTO_gcm128_tag(GCM128_CONTEXT *ctx, uint8_t *tag,
                                      size_t len);


#if defined(__cplusplus)
} /* extern C */
#endif

#endif /* OPENSSL_HEADER_MODES_INTERNAL_H */
