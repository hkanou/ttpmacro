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

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Shlwapi.h>
#include "ttmparse.h"
#include "ttmencfile.h"
#include "ttl.h"

CipherInfoP FCipher[NumFCipher];

static void EncryptSetIv(unsigned char*, DWORD, unsigned char *);
static void EncryptCloseHandle(CipherInfoP);

/**
 *	@retval 読み込みバイト数
 */
UINT Encrypt_lread(HANDLE FH, LPVOID lpBuffer, UINT uBytes, CipherInfoP cip)
{
	DWORD NumberOfBytesRead;
	DWORD pos;
	int num;
	unsigned char head[1];
	UINT i;

	if (cip->mem) {
		pos = cip->mem_pos;
		if (pos + uBytes > cip->mem_len) {
			NumberOfBytesRead = cip->mem_len - pos;
		} else {
			NumberOfBytesRead = uBytes;
		}
		memcpy(lpBuffer, cip->mem + pos, NumberOfBytesRead);
		cip->mem_pos += NumberOfBytesRead;
	} else {
		pos = SetFilePointer(FH, 0, NULL, FILE_CURRENT);
		if (pos == INVALID_SET_FILE_POINTER) {
			return 0;
		}
		pos -= ENC_FILE_OFFSET;
		BOOL Result = ReadFile(FH, lpBuffer, uBytes, &NumberOfBytesRead, NULL);
		if (Result == FALSE || NumberOfBytesRead == 0) {
			return 0;
		}
	}

	for (i = 0; i < uBytes; i++) {
		EncryptSetIv(cip->ctx->iv, pos / AES_BLOCK_SIZE, cip->iv);
		cip->ctx->num = 0;
		EVP_EncryptUpdate(cip->ctx, head, &num, (const unsigned char *)lpBuffer + i, 1);
		if (pos % AES_BLOCK_SIZE == 0) {
			memcpy((unsigned char *)lpBuffer + i, head, 1);
			pos++;
			continue;
		}
		cip->ctx->num = pos % AES_BLOCK_SIZE;
		if (EVP_EncryptUpdate(cip->ctx, (unsigned char *)lpBuffer + i,
							  &num, (const unsigned char *)lpBuffer + i, 1) == 0) {
			return 0;
		}
		pos++;
	}

	return NumberOfBytesRead;
}

/**
 *	@retval 書き込みバイト数
 */
UINT Encrypt_lwrite(HANDLE FH, LPVOID lpBuffer, UINT uBytes, CipherInfoP cip)
{
	DWORD NumberOfBytesWritten;
	DWORD pos;
	TStrVal tmpstr;
	int num;
	unsigned char head[1];
	UINT i;

	if (cip->mem) {
		pos = cip->mem_pos;
	} else {
		pos = SetFilePointer(FH, 0, NULL, FILE_CURRENT);
		if (pos == INVALID_SET_FILE_POINTER) {
			return 0;
		}
		pos -= ENC_FILE_OFFSET;
	}

	for (i = 0; i < uBytes; i++) {
		EncryptSetIv(cip->ctx->iv, pos / AES_BLOCK_SIZE, cip->iv);
		cip->ctx->num = 0;
		EVP_EncryptUpdate(cip->ctx, head, &num, (const unsigned char *)lpBuffer + i, 1);
		if (pos % AES_BLOCK_SIZE == 0) {
			tmpstr[i] = head[0];
			pos++;
			continue;
		}
		cip->ctx->num = pos % AES_BLOCK_SIZE;
		if (EVP_EncryptUpdate(cip->ctx, (unsigned char *)tmpstr + i,
							  &num, (const unsigned char *)lpBuffer + i, 1) == 0) {
			return 0;
		}
		pos++;
	}

	if (cip->mem) {
		if (cip->mem_pos + uBytes > cip->mem_max) {
			unsigned char *p;
			DWORD newsize;
			newsize = ((cip->mem_pos + uBytes) / ENC_FILE_BUF_SIZE + 1) * ENC_FILE_BUF_SIZE;
			if ((p = (unsigned char *)realloc(cip->mem, newsize)) == NULL) {
				return 0;
			}
			cip->mem = p;
			cip->mem_max += ENC_FILE_BUF_SIZE;
		}
		memcpy(cip->mem + cip->mem_pos, tmpstr, uBytes);
		cip->mem_pos += uBytes;
		if (cip->mem_pos > cip->mem_len) {
			cip->mem_len = cip->mem_pos;
		}
		NumberOfBytesWritten = uBytes;
	} else {
		BOOL result = WriteFile(FH, tmpstr, uBytes, &NumberOfBytesWritten, NULL);
		if (result == FALSE) {
			return 0;
		}
	}

	return NumberOfBytesWritten;
}

/*
 *	@param[in]	iOrigin
 *				@arg 0(FILE_BEGIN)
 *				@arg 1(FILE_CURRENT)
 *				@arg 2(FILE_END)
 *	@retval ファイル位置
 *	@retval HFILE_ERROR((HFILE)-1)	エラー
 */
LONG Encrypt_llseek(HANDLE FH, LONG lOffset, int iOrigin, CipherInfoP cip)
{
	DWORD cpos, epos, npos;

	if (cip->mem) {
		cpos = cip->mem_pos;
		epos = cip->mem_len;
		switch (iOrigin) {
		case FILE_BEGIN:
			npos = lOffset;
			break;
		case FILE_CURRENT:
			npos = cpos + lOffset;
			break;
		case FILE_END:
			npos = epos + lOffset;
			break;
		}
		if (npos < 0) {
			return HFILE_ERROR;
		} else if (npos > epos) {
			cip->mem_pos = epos;
			DWORD over;
			over = npos - epos;
			while (over-- > 0) {
				if (Encrypt_lwrite(FH, "\000", 1, cip) == 0) {
					break;
				}
			}
			if (over != 0) {
				return HFILE_ERROR;
			}
		}
		cip->mem_pos = npos;
		return npos;
	}

	if ((cpos = SetFilePointer(FH, 0, NULL, FILE_CURRENT)) == INVALID_SET_FILE_POINTER ||
		(epos = SetFilePointer(FH, 0, NULL, FILE_END))     == INVALID_SET_FILE_POINTER) {
		goto end;
	}
	switch (iOrigin) {
	case FILE_BEGIN:
		npos = lOffset + ENC_FILE_OFFSET;
		break;
	case FILE_CURRENT:
		npos = cpos + lOffset;
		break;
	case FILE_END:
		npos = epos + lOffset;
		break;
	}
	if (npos < ENC_FILE_OFFSET) {
		goto end;
	} else if (npos <= epos) {
		if (SetFilePointer(FH, npos, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
			goto end;
		}
	} else {
		if (SetFilePointer(FH, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) {
			goto end;
		}
		DWORD over;
		over = npos - epos;
		while (over-- > 0) {
			if (Encrypt_lwrite(FH, "\000", 1, cip) == 0) {
				break;
			}
		}
		if (over != 0) {
			goto end;
		}
	}
	return npos - ENC_FILE_OFFSET;

 end:
	SetFilePointer(FH, ENC_FILE_OFFSET, NULL, FILE_BEGIN);
	return HFILE_ERROR;
}

int EncryptFileConcat(LPCWSTR FName1W, LPCWSTR FName2W, TStrVal password1, TStrVal password2)
{
	int ret;
	HANDLE FH1 = NULL, FH2 = NULL;
	CipherInfoP cip1 = NULL, cip2 = NULL;
	BYTE buf[ENC_FILE_BUF_SIZE];
	DWORD rnum, wnum;
	LONG cpos, epos;

	ret = 3;
	if (password1[0] == 0) {
		if ((FH1 = CreateFileW(FName1W, GENERIC_WRITE, 0, NULL,
								OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE ||
			SetFilePointer(FH1, 0, NULL, FILE_END)  == INVALID_SET_FILE_POINTER) {
			goto end;
		}
	} else {
		FH1 = EncryptCreateFileW(FName1W, GENERIC_WRITE, 0, NULL,
								OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL, password1, &cip1);
		cip1->mem_pos = cip1->mem_len;
	}

	if (password2[0] == 0) {
		if ((FH2 = CreateFileW(FName2W, GENERIC_READ, FILE_SHARE_READ, NULL,
								OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
			goto end;
		}
	} else {
		if ((FH2 = EncryptCreateFileW(FName2W, GENERIC_READ, FILE_SHARE_READ, NULL,
								OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, password2, &cip2)) == INVALID_HANDLE_VALUE) {
			goto end;
		}
	}

	do {
		if (password2[0] == 0) {
			if (ReadFile(FH2, buf, sizeof(buf), &rnum, NULL) == FALSE) {
				ret = -4;
				goto end;
			}
		} else {
			rnum = Encrypt_lread(FH2, buf, sizeof(buf), cip2);
		}
		if (rnum > 0) {
			if (password1[0] == 0) {
				if (WriteFile(FH1, buf, rnum, &wnum, NULL) == FALSE) {
					ret = -5;
					goto end;
				}
			} else {
				wnum = Encrypt_lwrite(FH1, buf, rnum, cip1);
			}
			if (rnum != wnum) {
				ret = -5;
				break;
			}
		}
	} while (rnum >= sizeof(buf));

	cpos = win16_llseek(FH2, 0, FILE_CURRENT, cip2);
	epos = win16_llseek(FH2, 0, FILE_END, cip2);
	if (cpos == epos) {
		ret = 0;
	} else {
		ret = -4;
	}

 end:
	EncryptCloseHandle(cip1);
	EncryptCloseHandle(cip2);
	CloseHandle(FH1);
	CloseHandle(FH2);
	return ret;
}

HANDLE EncryptCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess,
						  DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
						  DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
						  HANDLE hTemplateFile, char *password, CipherInfoP *cip)
{
	HANDLE FH;
	unsigned char salt[PKCS5_SALT_LEN];
	BOOL newsalt;

	if ((*cip = (CipherInfoP)malloc(sizeof(CipherInfo))) == NULL) {
		return INVALID_HANDLE_VALUE;
	}
	(*cip)->ctx = NULL;
	(*cip)->mem = NULL;
	(*cip)->update = FALSE;

	if (PathFileExistsW(lpFileName)) {
		if (dwCreationDisposition == CREATE_ALWAYS ||
			dwCreationDisposition == TRUNCATE_EXISTING) {
			newsalt = TRUE;
		} else {
			newsalt = FALSE;
			dwDesiredAccess |= GENERIC_READ;
		}
	} else {
		newsalt = TRUE;
	}

	FH = CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
					 lpSecurityAttributes, dwCreationDisposition,
					 dwFlagsAndAttributes, hTemplateFile);
	if (FH == INVALID_HANDLE_VALUE) {
		free(*cip);
		*cip = NULL;
		return INVALID_HANDLE_VALUE;
	}

	if (newsalt) {
		if (RAND_bytes(salt, PKCS5_SALT_LEN) <= 0) {
			goto end;
		}
	} else {
		char mbuf[sizeof(ENC_FILE_MAGIC) - 1];
		DWORD num;
		if (ReadFile(FH, mbuf, sizeof(ENC_FILE_MAGIC) - 1, &num, NULL) == FALSE ||
			memcmp(mbuf, ENC_FILE_MAGIC, sizeof(ENC_FILE_MAGIC) - 1) != 0 ||
			ReadFile(FH, salt, PKCS5_SALT_LEN, &num, NULL) == FALSE) {
			goto end;
		}
	}

	// 編集用
	unsigned char tmpkeyiv[ENC_FILE_IKLEN + ENC_FILE_IVLEN];
	if (PKCS5_PBKDF2_HMAC(password, strlen(password),
						  (const unsigned char *)&salt, PKCS5_SALT_LEN,
						  ENC_FILE_ITER, (EVP_MD *)ENC_FILE_DIGEST,
						  ENC_FILE_IKLEN + ENC_FILE_IVLEN, tmpkeyiv) != 1) {
		goto end;
	}
	memcpy((*cip)->iv, tmpkeyiv + ENC_FILE_IKLEN, ENC_FILE_IVLEN);
	if (((*cip)->ctx = EVP_CIPHER_CTX_new()) == NULL ||
		EVP_EncryptInit_ex((*cip)->ctx, ENC_FILE_CIPHER, NULL, tmpkeyiv, (*cip)->iv) != 1 ||
		EVP_CIPHER_CTX_set_padding((*cip)->ctx, 0) != 1) {
		goto end;
	}

	// 書き込み用
	if (RAND_bytes(salt, PKCS5_SALT_LEN) <= 0 ||
		PKCS5_PBKDF2_HMAC(password, strlen(password),
						  (const unsigned char *)&salt, PKCS5_SALT_LEN,
						  ENC_FILE_ITER, (EVP_MD *)ENC_FILE_DIGEST,
						  ENC_FILE_IKLEN + ENC_FILE_IVLEN, tmpkeyiv) != 1) {
		goto end;
	}
	memcpy((*cip)->new_salt, salt, PKCS5_SALT_LEN);
	memcpy((*cip)->new_iv, tmpkeyiv + ENC_FILE_IKLEN, ENC_FILE_IVLEN);
	if (((*cip)->new_ctx = EVP_CIPHER_CTX_new()) == NULL ||
		EVP_EncryptInit_ex((*cip)->new_ctx, ENC_FILE_CIPHER, NULL, tmpkeyiv, (*cip)->new_iv) != 1 ||
		EVP_CIPHER_CTX_set_padding((*cip)->new_ctx, 0) != 1) {
		goto end;
	}

	if (dwDesiredAccess != GENERIC_READ) {
		DWORD readnum;
		BOOL result;

		if (((*cip)->mem = (unsigned char *)malloc(ENC_FILE_BUF_SIZE)) == NULL) {
			goto end;
		}
		(*cip)->mem_max = ENC_FILE_BUF_SIZE;
		(*cip)->mem_len = 0;
		(*cip)->mem_pos = 0;

		while (1) {
			result = ReadFile(FH, (*cip)->mem + (*cip)->mem_pos, ENC_FILE_BUF_SIZE, &readnum, NULL);
			if (result == FALSE || readnum == 0) {
				break;
			}
			(*cip)->mem_pos += readnum;
			if ((*cip)->mem_pos == (*cip)->mem_max) {
				unsigned char *p;
				if ((p = (unsigned char *)realloc((*cip)->mem, ENC_FILE_BUF_SIZE)) == NULL) {
					goto end;
				}
				(*cip)->mem = p;
				(*cip)->mem_max += ENC_FILE_BUF_SIZE;
			}
		}
		(*cip)->mem_len = (*cip)->mem_pos;
		(*cip)->mem_pos = 0;

		if (readnum != 0 ||
			SetFilePointer(FH, ENC_FILE_OFFSET, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
			goto end;
		}
	}

	return FH;

 end:
	EncryptCloseHandle(*cip);
	CloseHandle(FH);
	return INVALID_HANDLE_VALUE;
}

static void EncryptSetIv(unsigned char *iv_ctx, DWORD counter, unsigned char *iv_base)
{
	int n;
	ptrdiff_t nval;

	memcpy(iv_ctx, iv_base, 16);
	for (n = 15; counter != 0 && n >= 0; n--) {
		nval = iv_ctx[n] + counter;
		iv_ctx[n] = nval & 0xff;
		counter = nval >> 8;
	}
}

void EncryptCloseHandle(int fhi)
{
	HANDLE FH;
	
	if ((FH = HandleGet(fhi)) == INVALID_HANDLE_VALUE ||
		FCipher[fhi] == NULL) {
		return;
	}
	EncryptCloseHandle(FCipher[fhi]);
}

static void EncryptCloseHandle(CipherInfoP cip)
{
	if (cip) {
		EVP_CIPHER_CTX_free(cip->ctx);
		EVP_CIPHER_CTX_free(cip->new_ctx);
		free(cip->mem);
		free(cip);
		cip = NULL;
	}
}

int EncryptCloseFile(int fhi)
{
	HANDLE FH;
	
	if ((FH = HandleGet(fhi)) == INVALID_HANDLE_VALUE ||
		FCipher[fhi] == NULL) {
		return 0;
	}
	return EncryptCloseFile(FH, FCipher[fhi]);
}

int EncryptCloseFile(HANDLE FH, CipherInfoP cip)
{
	EVP_CIPHER_CTX *cur_ctx, *new_ctx;
	unsigned char *mem;
	DWORD pos, len;
	unsigned char *cur_iv, *new_iv;
	unsigned char buf[ENC_FILE_BUF_SIZE];
	int buflen;
	int readnum;
	DWORD writenum;
	BOOL result;

	static const char magic[] = "Salted__";
	int magic_len = sizeof(magic) - 1;

	if (cip == NULL) {
		return 0;
	}

	mem = cip->mem;
	if (mem && cip->update == TRUE) {
		if (SetFilePointer(FH, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
			WriteFile(FH, ENC_FILE_MAGIC, sizeof(ENC_FILE_MAGIC) - 1, &writenum, NULL) == FALSE ||
			WriteFile(FH, cip->new_salt, PKCS5_SALT_LEN, &writenum, NULL) == FALSE){
			return -1;
		}

		cur_ctx = cip->ctx;
		cur_iv  = cip->iv;
		new_ctx = cip->new_ctx;
		new_iv  = cip->new_iv;
		EncryptSetIv(cur_ctx->iv, 0, cur_iv);
		EncryptSetIv(new_ctx->iv, 0, new_iv);
		cur_ctx->num = 0;
		new_ctx->num = 0;

		pos = 0;
		len = cip->mem_len;
		while (pos < len) {
			if (len - pos > ENC_FILE_BUF_SIZE) {
				readnum = ENC_FILE_BUF_SIZE;
			} else {
				readnum = len - pos;
			}
			memcpy(buf, mem + pos, readnum);
			pos += readnum;
			if (EVP_EncryptUpdate(cur_ctx, buf, &buflen, buf, readnum) == 0 ||
				EVP_EncryptUpdate(new_ctx, buf, &buflen, buf, readnum) == 0) {
				break;
			}
			result = WriteFile(FH, buf, readnum, &writenum, NULL);
			if (result == FALSE || readnum != writenum) {
				break;
			}
		}
	}
	EncryptCloseHandle(cip);

	return 0;
}
