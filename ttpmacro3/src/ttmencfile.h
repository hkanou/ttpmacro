/*
 * Copyright (C) 2024- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* TTMACRO.EXE, file encryption */

#pragma once

#define LIBRESSL_DISABLE_OVERRIDE_WINCRYPT_DEFINES_WARNING
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/buffer.h>
#include "ttmparse.h"

#define __BEGIN_HIDDEN_DECLS
#define __END_HIDDEN_DECLS
#include "../crypto/evp/evp_local.h"
#undef __BEGIN_HIDDEN_DECLS
#undef __END_HIDDEN_DECLS

#define ENC_FILE_MAGIC  	"Salted__"
#define ENC_FILE_CIPHER 	EVP_aes_256_ctr()
#define ENC_FILE_DIGEST 	EVP_sha256()
#define ENC_FILE_ITER		10000
#define ENC_FILE_IKLEN 		32
#define ENC_FILE_IVLEN 		16
#define ENC_FILE_OFFSET		16 			// "Salted__"(8ï∂éö) Å{ salt(8ï∂éö)
#define ENC_FILE_BUF_SIZE	1024

typedef struct {
	EVP_CIPHER_CTX *ctx;
	unsigned char iv[AES_BLOCK_SIZE];
	EVP_CIPHER_CTX *new_ctx;
	unsigned char new_iv[AES_BLOCK_SIZE];
	unsigned char new_salt[PKCS5_SALT_LEN];
	BOOL update;
	unsigned char *mem;
	DWORD mem_pos;
	DWORD mem_len;
	DWORD mem_max;
} CipherInfo, *CipherInfoP;

#define NumFCipher 16
extern CipherInfoP FCipher[NumFCipher];

extern HANDLE HandleGet(int);
extern LONG win16_llseek(HANDLE, LONG, int, CipherInfoP);

UINT Encrypt_lread(HANDLE, LPVOID, UINT, CipherInfoP);
UINT Encrypt_lwrite(HANDLE, LPVOID, UINT, CipherInfoP);
LONG Encrypt_llseek(HANDLE, LONG, int, CipherInfoP);
int EncryptFileConcat(LPCWSTR, LPCWSTR, TStrVal, TStrVal);
HANDLE EncryptCreateFileW(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE, char *, CipherInfoP *);
void EncryptCloseHandle(int);
int EncryptCloseFile(HANDLE, CipherInfoP);
int EncryptCloseFile(int);
