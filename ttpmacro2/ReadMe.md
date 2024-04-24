# setpassword、getpasswordへの暗号化機能追加パッチ

- 修正内容
  setpasswordとgetpasswordに\<encryptstr\>オプション(省略可能)を追加  
  <encryptstr>が指定された場合は、\<encryptstr\>を元に\<strval\>をAES-256-CTRで暗号化/復号する  
- 書式  
  setpassword \<filename\> \<password name\> \<strval\> [\<encryptstr\>]  
  getpassword \<filename\> \<password name\> \<strvar\> [\<encryptstr\>]  

# 詳細、使用方法

[setpassword](http://htmlpreview.github.io/?https://github.com/hkanou/ttpmacro/blob/main/ttpmacro2/doc/setpassword.html)  
[getpassword](http://htmlpreview.github.io/?https://github.com/hkanou/ttpmacro/blob/main/ttpmacro2/doc/getpassword.html)  

# パッチ

## % diff -rup ttpmacro_org/ttl.cpp ttpmacro_enc/ttl.cpp
```diff
--- ttpmacro_org/ttl.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro_enc/ttl.cpp	2024-04-23 23:11:17.500580300 +0900
@@ -2524,10 +2524,10 @@ static WORD TTLGetIPv6Addr(void)
 	return Err;
 }
 
-// setpassword 'password.dat' 'mypassword' passowrd
+// setpassword 'password.dat' 'mypassword' passowrd [encryptstr]
 static WORD TTLSetPassword(void)
 {
-	TStrVal FileNameStr, KeyStr, PassStr;
+	TStrVal FileNameStr, KeyStr, PassStr, EncryptStr;
 	char Temp[512];
 	WORD Err;
 	int result = 0;  /* failure */
@@ -2536,6 +2536,13 @@ static WORD TTLSetPassword(void)
 	GetStrVal(FileNameStr, &Err);   // ファイル名
 	GetStrVal(KeyStr, &Err);  // キー名
 	GetStrVal(PassStr, &Err);  // パスワード
+	EncryptStr[0] = 0;
+	if (CheckParameterGiven()) {
+		GetStrVal(EncryptStr, &Err);  // 暗号化文字列
+		if ((Err == 0) && (EncryptStr[0] == 0)) {
+			return ErrSyntax;
+		}
+	}
 	if ((Err==0) && (GetFirstChar()!=0))
 		Err = ErrSyntax;
 	if (Err!=0) return Err;
@@ -2550,7 +2557,10 @@ static WORD TTLSetPassword(void)
 	GetAbsPath(FileNameStr, sizeof(FileNameStr));
 
 	// パスワードを暗号化する。
-	Encrypt(PassStr, Temp);
+	if (Encrypt(PassStr, Temp, EncryptStr) != 0) {
+		SetResult(result);
+		return Err;
+	}
 
 	if (WritePrivateProfileString("Password", KeyStr, Temp, FileNameStr) != 0)
 		result = 1;  /* success */
```

## % diff -rup ttpmacro_org/ttl_gui.cpp ttpmacro/ttl_gui.cpp
```diff
--- ttpmacro_org/ttl_gui.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro_enc/ttl_gui.cpp	2024-04-23 23:11:17.500772900 +0900
@@ -194,7 +194,7 @@ WORD TTLFilenameBox()
 
 WORD TTLGetPassword()
 {
-	TStrVal Str, Str2, Temp2;
+	TStrVal Str, Str2, Temp2, EncryptStr;
 	wchar_t Temp[512];
 	WORD Err;
 	TVarId VarId;
@@ -204,12 +204,19 @@ WORD TTLGetPassword()
 	GetStrVal(Str,&Err);  // ファイル名
 	GetStrVal(Str2,&Err);  // キー名
 	GetStrVar(&VarId,&Err);  // パスワード更新時にパスワードを格納する変数
-	if ((Err==0) && (GetFirstChar()!=0))
-		Err = ErrSyntax;
-	if (Err!=0) return Err;
 	SetStrVal(VarId,"");
 	if (Str[0]==0) return Err;
 	if (Str2[0]==0) return Err;
+	EncryptStr[0] = 0;
+	if (CheckParameterGiven()) {
+		GetStrVal(EncryptStr, &Err);  // 暗号化文字列
+		if ((Err == 0) && (EncryptStr[0] == 0)) {
+			return ErrSyntax;
+		}
+	}
+	if ((Err==0) && (GetFirstChar()!=0))
+		Err = ErrSyntax;
+	if (Err!=0) return Err;
 
 	GetAbsPath(Str,sizeof(Str));
 
@@ -223,16 +230,18 @@ WORD TTLGetPassword()
 		WideCharToUTF8(input_string, NULL, Temp2, &Temp2_len);
 		if (Temp2[0]!=0) {
 			char TempA[512];
-			Encrypt(Temp2, TempA);
-			if (WritePrivateProfileStringW(L"Password", (wc)Str2, (wc)TempA, wc::fromUtf8(Str)) != 0) {
-				result = 1;  /* success */
+			if (Encrypt(Temp2, TempA, EncryptStr) == 0) {
+				if (WritePrivateProfileStringW(L"Password", (wc)Str2, (wc)TempA, wc::fromUtf8(Str)) != 0) {
+					result = 1;  /* success */
+				}
 			}
 		}
 	}
 	else {// password exist
 		u8 TempU8 = Temp;
-		Decrypt((PCHAR)(const char *)TempU8,Temp2);
-		result = 1;  /* success */
+		if (Decrypt((PCHAR)(const char *)TempU8, Temp2, EncryptStr) == 0) {
+			result = 1;  /* success */
+		}
 	}
 
 	if (result == 1) {
```

## % diff -rup ttpmacro_org/ttmenc.c ttpmacro/ttmenc.c
```diff
--- ttpmacro_org/ttmenc.c	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro_enc/ttmenc.c	2024-04-23 23:11:17.500927600 +0900
@@ -33,6 +33,123 @@
 #include <stdlib.h>
 #include <string.h>
 
+#define LIBRESSL_DISABLE_OVERRIDE_WINCRYPT_DEFINES_WARNING
+
+#include <openssl/sha.h>
+#include <openssl/evp.h>
+#include <openssl/rand.h>
+#include "ttmdef.h"
+
+#define PWD_MAX_LEN 300
+#define PWD_CIPHER EVP_aes_256_ctr()
+#define ENC_IN_LEN 508
+#define PBKDF2_DIGEST EVP_sha256()
+#define PBKDF2_ITER_DEFAULT 10000
+#define PBKDF2_IKLEN 32
+#define PBKDF2_IVLEN 16
+
+static int EncDecPWD(PCHAR InStr, PCHAR OutStr, const char *EncryptStr, int encode)
+{
+	static const char magic[] = "Salted__";
+	int magic_len = sizeof(magic) - 1;
+	unsigned char salt[PKCS5_SALT_LEN];
+	BIO *b64 = NULL, *mem = NULL, *bio = NULL, *benc = NULL;
+	int in_len;
+	int ret = -1;
+	
+	if ((b64 = BIO_new(BIO_f_base64())) == NULL ||
+		(mem = BIO_new(BIO_s_mem())) == NULL ||
+		(bio = BIO_push(b64, mem)) == NULL) {
+		goto end;
+	}
+	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
+	in_len = strlen(InStr);
+	
+	if (encode) {
+		if (in_len > PWD_MAX_LEN ||
+			BIO_write(bio, magic, magic_len) != magic_len ||
+			RAND_bytes(salt, PKCS5_SALT_LEN) <= 0 ||
+			BIO_write(bio, (char *)salt, PKCS5_SALT_LEN) != PKCS5_SALT_LEN) {
+			goto end;
+		}
+	} else {
+		char mbuf[sizeof(magic) - 1];
+		if (in_len != ENC_IN_LEN ||
+			BIO_write(mem, InStr, in_len) != in_len ||
+			BIO_flush(mem) != 1 ||
+			BIO_read(bio, mbuf, magic_len) != magic_len ||
+			memcmp(mbuf, magic, magic_len) != 0 ||
+			BIO_read(bio, salt, PKCS5_SALT_LEN) != PKCS5_SALT_LEN) {
+			goto end;
+		}
+	}
+	
+	unsigned char tmpkeyiv[EVP_MAX_KEY_LENGTH + EVP_MAX_IV_LENGTH];
+	unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
+	int iklen = PBKDF2_IKLEN;
+	int ivlen = PBKDF2_IVLEN;
+	if (PKCS5_PBKDF2_HMAC(EncryptStr, strlen(EncryptStr),
+						  (const unsigned char *)&salt, PKCS5_SALT_LEN,
+						  PBKDF2_ITER_DEFAULT, (EVP_MD *)PBKDF2_DIGEST,
+						  iklen + ivlen, tmpkeyiv) != 1) {
+		goto end;
+	}
+	memcpy(key, tmpkeyiv, iklen);
+	memcpy(iv, tmpkeyiv + iklen, ivlen);
+	
+	EVP_CIPHER_CTX *ctx;
+	if ((benc = BIO_new(BIO_f_cipher())) == NULL ||
+		BIO_get_cipher_ctx(benc, &ctx) != 1 ||
+		EVP_CipherInit_ex(ctx, PWD_CIPHER, NULL, key, iv, encode) != 1 ||
+		(bio = BIO_push(benc, bio))  == NULL) {
+		goto end;
+	}
+
+	unsigned char hash[MaxStrLen + PKCS5_SALT_LEN];
+	unsigned char hbuf[SHA512_DIGEST_LENGTH];
+	int enc_len;
+	enc_len = strlen(EncryptStr);
+	memcpy(hash, EncryptStr, enc_len);
+	memcpy(hash + enc_len, salt, PKCS5_SALT_LEN);
+	SHA512(hash, enc_len + PKCS5_SALT_LEN, hbuf);
+	
+	if (encode) {
+		unsigned char tmpbuf[MaxStrLen];
+		int len;
+		memcpy(tmpbuf, InStr, in_len);
+		memset(tmpbuf + in_len, 0x00, PWD_MAX_LEN - in_len);
+		tmpbuf[PWD_MAX_LEN] = 0;
+		if (BIO_write(bio, tmpbuf, PWD_MAX_LEN + 1) != PWD_MAX_LEN + 1 ||
+			BIO_write(bio, hbuf, SHA512_DIGEST_LENGTH) != SHA512_DIGEST_LENGTH ||
+			BIO_flush(bio) != 1 ||
+			(len = BIO_read(mem, OutStr, MaxStrLen)) <= 0) {
+			goto end;
+		}
+		OutStr[len] = 0;
+		ret = 0;
+	} else {
+		char tmpbuf[MaxStrLen];
+		unsigned char hbuf_in[SHA512_DIGEST_LENGTH];
+		if (BIO_read(bio, tmpbuf, PWD_MAX_LEN + 1) != PWD_MAX_LEN + 1 ||
+			BIO_read(bio, hbuf_in, SHA512_DIGEST_LENGTH) != SHA512_DIGEST_LENGTH ||
+			memcmp(hbuf, hbuf_in, SHA512_DIGEST_LENGTH) != 0) {
+			goto end;
+		}
+		memcpy(OutStr, tmpbuf, PWD_MAX_LEN + 1);
+		ret = 0;
+	}
+
+ end:
+	if (ret != 0) {
+		OutStr[0] = 0;
+	}
+	BIO_free(benc);
+	BIO_free(mem);
+	BIO_free(b64);
+
+	return ret;
+}
+
 BOOL EncSeparate(const char *Str, int *i, LPBYTE b)
 {
 	int cptr, bptr;
@@ -44,7 +161,7 @@ BOOL EncSeparate(const char *Str, int *i
 	}
 	bptr = *i % 8;
 	d = ((BYTE)Str[cptr] << 8) |
-	     (BYTE)Str[cptr+1];
+		 (BYTE)Str[cptr+1];
 	*b = (BYTE)((d >> (10-bptr)) & 0x3f);
 
 	*i = *i + 6;
@@ -81,34 +198,44 @@ BYTE EncCharacterize(BYTE c, LPBYTE b)
 	return d;
 }
 
-void Encrypt(const char *InStr, PCHAR OutStr)
+int Encrypt(const char *InStr, PCHAR OutStr, PCHAR EncryptStr)
 {
+
 	int i, j;
 	BYTE b, r, r2;
 
 	OutStr[0] = 0;
 	if (InStr[0]==0) {
-		return;
+		return 0;
 	}
-	srand(LOWORD(GetTickCount()));
-	r = (BYTE)(rand() & 0x3f);
-	r2 = (~r) & 0x3f;
-	OutStr[0] = r;
-	i = 0;
-	j = 1;
-	while (EncSeparate(InStr,&i,&b)) {
+
+	if (EncryptStr[0] != 0) {
+		if (EncDecPWD((PCHAR)InStr, OutStr, EncryptStr, 1) != 0) {
+			return -1;
+		}
+	} else {
+		srand(LOWORD(GetTickCount()));
 		r = (BYTE)(rand() & 0x3f);
-		OutStr[j++] = (b + r) & 0x3f;
-		OutStr[j++] = r;
-	}
-	OutStr[j++] = r2;
-	OutStr[j] = 0;
-	i = 0;
-	b = 0x21;
-	while (i < j) {
-		OutStr[i] = EncCharacterize(OutStr[i],&b);
-		i++;
+		r2 = (~r) & 0x3f;
+		OutStr[0] = r;
+		i = 0;
+		j = 1;
+		while (EncSeparate(InStr,&i,&b)) {
+			r = (BYTE)(rand() & 0x3f);
+			OutStr[j++] = (b + r) & 0x3f;
+			OutStr[j++] = r;
+		}
+		OutStr[j++] = r2;
+		OutStr[j] = 0;
+		i = 0;
+		b = 0x21;
+		while (i < j) {
+			OutStr[i] = EncCharacterize(OutStr[i],&b);
+			i++;
+		}
 	}
+
+	return 0;
 }
 
 void DecCombine(PCHAR Str, int *i, BYTE b)
@@ -165,7 +292,7 @@ BYTE DecCharacter(BYTE c, LPBYTE b)
 	return d;
 }
 
-void Decrypt(PCHAR InStr, PCHAR OutStr)
+int Decrypt(PCHAR InStr, PCHAR OutStr, PCHAR EncryptStr)
 {
 	int i, j, k;
 	BYTE b;
@@ -174,20 +301,29 @@ void Decrypt(PCHAR InStr, PCHAR OutStr)
 	OutStr[0] = 0;
 	j = strlen(InStr);
 	if (j==0) {
-		return;
+		return 0;
 	}
-	b = 0x21;
-	for (i=0 ; i < j ; i++) {
-		Temp[i] = DecCharacter(InStr[i],&b);
-	}
-	if ((Temp[0] ^ Temp[j-1]) != (BYTE)0x3f) {
-		return;
-	}
-	i = 1;
-	k = 0;
-	while (i < j-2) {
-		Temp[i] = ((BYTE)Temp[i] - (BYTE)Temp[i+1]) & (BYTE)0x3f;
-		DecCombine(OutStr,&k,Temp[i]);
-		i = i + 2;
+
+	if (EncryptStr[0] != 0) {
+		if (EncDecPWD((PCHAR)InStr, OutStr, EncryptStr, 0) != 0) {
+			return -1;
+		}
+	} else {
+		b = 0x21;
+		for (i=0 ; i < j ; i++) {
+			Temp[i] = DecCharacter(InStr[i],&b);
+		}
+		if ((Temp[0] ^ Temp[j-1]) != (BYTE)0x3f) {
+			return 0;
+		}
+		i = 1;
+		k = 0;
+		while (i < j-2) {
+			Temp[i] = ((BYTE)Temp[i] - (BYTE)Temp[i+1]) & (BYTE)0x3f;
+			DecCombine(OutStr,&k,Temp[i]);
+			i = i + 2;
+		}
 	}
+
+	return 0;
 }
```

## % diff -rup ttpmacro_org/ttmenc.h ttpmacro/ttmenc.h
```diff
--- ttpmacro_org/ttmenc.h	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro_enc/ttmenc.h	2024-04-23 23:11:17.501073500 +0900
@@ -33,8 +33,8 @@
 extern "C" {
 #endif
 
-void Encrypt(const char *InStr, PCHAR OutStr);
-void Decrypt(PCHAR InStr, PCHAR OutStr);
+int Encrypt(const char *InStr, PCHAR OutStr, PCHAR EncryptStr);
+int Decrypt(PCHAR InStr, PCHAR OutStr, PCHAR EncryptStr);
 
 #ifdef __cplusplus
 }
```

# ビルド方法

- "追加のインクルード ディレクトリ"に下記を追加  
  $(SolutionDir)..\libs\libressl\include  
- "追加のライブラリ ディレクトリ"に下記を追加  
  $(SolutionDir)..\libs\libressl\lib  
- "追加の依存ファイル"の先頭に下記を追加  
  common_static.lib  
  cryptod.lib  
  Bcrypt.lib  
