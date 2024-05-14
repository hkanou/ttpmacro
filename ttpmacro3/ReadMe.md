# ファイル操作コマンド暗号化機能追加パッチ

    **<ins>※ 試作中</ins>**  

    暗号化されたファイルの読み書き等を可能とするパッチです。  
    
    filecreate \<file handle\> \<filename\> **<ins>['password=abcd']</ins>**  
    fileopen \<file handle\> \<filename\> \<append flag\> [<readonly flag>] **<ins>['password=abcd']</ins>**  
    fileconcat \<file1\> \<file2\> ['password1=abcd'] **<ins>['password2=abcd']</ins>**  
    filetruncate \<filename\> \<size\> **<ins>['password=abcd']</ins>**  
    
    下記のコマンドが暗号化機能対応になります。  
  
    fileread  
    filereadln  
    filewrite  
    filewriteln  
    fileseek  
    fileseekback  
    filemarkptr  
    filestrseek  
    filestrseek2  
    fileclose  

# パッチ  

## % diff -rup ttl.cpp.org ttl.cpp
```diff
--- ttl.cpp.org	2024-02-28 09:02:00.000000000 +0900
+++ ttl.cpp	2024-05-12 19:54:28.315669600 +0900
@@ -43,6 +43,7 @@
 #include "ttmlib.h"
 #include "ttlib.h"
 #include "ttmenc.h"
+#include "ttmencfile.h"
 #include "tttypes.h"
 #include "ttmonig.h"
 #include <shellapi.h>
@@ -107,7 +108,7 @@ static void HandleInit(void)
  *	@retval	ファイルハンドルインデックス(0～)
  *			-1のときエラー
  */
-static int HandlePut(HANDLE FH)
+static int HandlePut(HANDLE FH, CipherInfoP cip)
 {
 	int i;
 	if (FH == INVALID_HANDLE_VALUE) {
@@ -117,13 +118,14 @@ static int HandlePut(HANDLE FH)
 		if (FHandle[i] == INVALID_HANDLE_VALUE) {
 			FHandle[i] = FH;
 			FPointer[i] = 0;
+			FCipher[i] = cip;
 			return i;
 		}
 	}
 	return -1;
 }
 
-static HANDLE HandleGet(int fhi)
+HANDLE HandleGet(int fhi)
 {
 	if (fhi < 0 || _countof(FHandle) < fhi) {
 		return INVALID_HANDLE_VALUE;
@@ -139,10 +141,14 @@ static void HandleFree(int fhi)
 /**
  *	@retval 読み込みバイト数
  */
-static UINT win16_lread(HANDLE hFile, LPVOID lpBuffer, UINT uBytes)
+static UINT win16_lread(HANDLE FH, LPVOID lpBuffer, UINT uBytes, CipherInfoP cip)
 {
+	if (cip) {
+		return Encrypt_lread(FH, lpBuffer, uBytes, cip);
+	}
+	
 	DWORD NumberOfBytesRead;
-	BOOL Result = ReadFile(hFile, lpBuffer, uBytes, &NumberOfBytesRead, NULL);
+	BOOL Result = ReadFile(FH, lpBuffer, uBytes, &NumberOfBytesRead, NULL);
 	if (Result == FALSE) {
 		return 0;
 	}
@@ -152,10 +158,14 @@ static UINT win16_lread(HANDLE hFile, LP
 /**
  *	@retval 書き込みバイト数
  */
-static UINT win16_lwrite(HANDLE hFile, const char*buf, UINT length)
+static UINT win16_lwrite(HANDLE FH, LPVOID lpBuffer, UINT uBytes, CipherInfoP cip)
 {
+	if (cip) {
+		return Encrypt_lwrite(FH, lpBuffer, uBytes, cip);
+	}
+
 	DWORD NumberOfBytesWritten;
-	BOOL result = WriteFile(hFile, buf, length, &NumberOfBytesWritten, NULL);
+	BOOL result = WriteFile(FH, lpBuffer, uBytes, &NumberOfBytesWritten, NULL);
 	if (result == FALSE) {
 		return 0;
 	}
@@ -171,9 +181,13 @@ static UINT win16_lwrite(HANDLE hFile, c
  *	@retval HFILE_ERROR((HFILE)-1)	エラー
  *	@retval INVALID_SET_FILE_POINTER((DWORD)-1) エラー
  */
-static LONG win16_llseek(HANDLE hFile, LONG lOffset, int iOrigin)
+LONG win16_llseek(HANDLE FH, LONG lOffset, int iOrigin, CipherInfoP cip)
 {
-	DWORD pos = SetFilePointer(hFile, lOffset, NULL, iOrigin);
+	if (cip) {
+		return Encrypt_llseek(FH, lOffset, iOrigin, cip);
+	}
+
+	DWORD pos = SetFilePointer(FH, lOffset, NULL, iOrigin);
 	if (pos == INVALID_SET_FILE_POINTER) {
 		return HFILE_ERROR;
 	}
@@ -293,6 +307,15 @@ void EndTTL()
 {
 	int i;
 
+	if (FHandle[0] != 0) {
+		for (i = 0; i< NumFHandle; i++) {
+			if (FHandle[i] != INVALID_HANDLE_VALUE) {
+				EncryptCloseFile(i);
+				CloseHandle(FHandle[i]);
+			}
+		}
+	}
+
 	CloseStatDlg();
 
 	if (DirHandle[0] != 0) {	// InitTTL() されずに EndTTL() 時対策
@@ -1234,6 +1257,7 @@ static WORD TTLFileClose(void)
 	if ((Err==0) && (GetFirstChar()!=0))
 		Err = ErrSyntax;
 	if (Err!=0) return Err;
+	EncryptCloseFile(fhi);
 	CloseHandle(FH);
 	HandleFree(fhi);
 	return Err;
@@ -1243,10 +1267,31 @@ static WORD TTLFileConcat(void)
 {
 	WORD Err;
 	TStrVal FName1, FName2;
+	TStrVal password1, password2;
 
 	Err = 0;
+	password1[0] = 0;
+	password2[0] = 0;
 	GetStrVal(FName1,&Err);
 	GetStrVal(FName2,&Err);
+	while (CheckParameterGiven()) {
+		TStrVal StrTmp;
+		GetStrVal(StrTmp, &Err);
+		if (Err == 0) {
+			wchar_t dummy[MaxStrLen];
+			if (_wcsnicmp(wc::fromUtf8(StrTmp), L"password1=", 10) == 0) {
+				if (swscanf_s(wc::fromUtf8(StrTmp), L"%[^=]=%hs", dummy, MaxStrLen, password1, MaxStrLen) != 2) {
+					Err = ErrSyntax;
+				}
+			} else if (_wcsnicmp(wc::fromUtf8(StrTmp), L"password2=", 10) == 0) {
+				if (swscanf_s(wc::fromUtf8(StrTmp), L"%[^=]=%hs", dummy, MaxStrLen, password2, MaxStrLen) != 2) {
+					Err = ErrSyntax;
+				}
+			}
+		} else {
+			break;
+		}
+	}
 	if ((Err==0) &&
 	    ((strlen(FName1)==0) ||
 	     (strlen(FName2)==0) ||
@@ -1271,6 +1316,13 @@ static WORD TTLFileConcat(void)
 	}
 
 	wc FName1W = wc::fromUtf8(FName1);
+	wc FName2W = wc::fromUtf8(FName2);
+
+	if (password1[0] != 0 || password2[0] != 0) {
+		SetResult(EncryptFileConcat(FName1W, FName2W, password1, password2));
+		return Err;
+	}
+
 	HANDLE FH1 = CreateFileW(FName1W,
 							 GENERIC_WRITE, 0, NULL,
 							 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
@@ -1281,7 +1333,6 @@ static WORD TTLFileConcat(void)
 	SetFilePointer(FH1, 0, NULL, FILE_END);
 
 	int result = 0;
-	wc FName2W = wc::fromUtf8(FName2);
 	HANDLE FH2 = CreateFileW(FName2W,
 							 GENERIC_READ, FILE_SHARE_READ, NULL,
 							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
@@ -1362,10 +1413,25 @@ static WORD TTLFileCreate(void)
 	HANDLE FH;
 	int fhi;
 	TStrVal FName;
+	TStrVal password;
+	CipherInfoP cip = NULL;
 
 	Err = 0;
+	password[0] = 0;
 	GetIntVar(&VarId, &Err);
 	GetStrVal(FName, &Err);
+	if (CheckParameterGiven()) {
+		TStrVal StrTmp;
+		GetStrVal(StrTmp, &Err);
+		if (Err == 0) {
+			if (_wcsnicmp(wc::fromUtf8(StrTmp), L"password=", 9) == 0) {
+				wchar_t dummy[MaxStrLen];
+				if (swscanf_s(wc::fromUtf8(StrTmp), L"%[^=]=%hs", dummy, MaxStrLen, password, MaxStrLen) != 2) {
+					Err = ErrSyntax;
+				}
+			}
+		}
+	}
 	if ((Err==0) &&
 	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
 		Err = ErrSyntax;
@@ -1380,21 +1446,39 @@ static WORD TTLFileCreate(void)
 		return Err;
 	}
 	wc FNameW = wc::fromUtf8(FName);
-	// TTL のファイルハンドルは filelock でロックするので、
-	// dwShareMode での共有モードは Read/Write とも有効にする。
-	FH = CreateFileW(FNameW,
-					 GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
-					 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
-	if (FH == INVALID_HANDLE_VALUE) {
-		SetResult(2);
-	}
-	else {
-		SetResult(0);
-	}
-	fhi = HandlePut(FH);
-	SetIntVal(VarId, fhi);
-	if (fhi == -1) {
-		CloseHandle(FH);
+
+	if (password[0] == 0) {
+		// TTL のファイルハンドルは filelock でロックするので、
+		// dwShareMode での共有モードは Read/Write とも有効にする。
+		FH = CreateFileW(FNameW,
+						 GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
+						 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
+		if (FH == INVALID_HANDLE_VALUE) {
+			SetResult(2);
+		}
+		else {
+			SetResult(0);
+		}
+		fhi = HandlePut(FH, NULL);
+		SetIntVal(VarId, fhi);
+		if (fhi == -1) {
+			CloseHandle(FH);
+		}
+	} else {
+		FH = EncryptCreateFileW(FNameW,
+								GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
+								CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL, password, &cip);
+		if (FH == INVALID_HANDLE_VALUE) {
+			SetResult(2);
+		} else {
+			SetResult(0);
+		}
+		fhi = HandlePut(FH, cip);
+		SetIntVal(VarId, fhi);
+		if (fhi == -1) {
+			EncryptCloseHandle(fhi);
+			CloseHandle(FH);
+		}
 	}
 	return Err;
 }
@@ -1442,7 +1526,7 @@ static WORD TTLFileMarkPtr(void)
 	if ((Err==0) && (GetFirstChar()!=0))
 		Err = ErrSyntax;
 	if (Err!=0) return Err;
-	pos = win16_llseek(FH,0,1);	 /* mark current pos */
+	pos = win16_llseek(FH, 0, FILE_CURRENT, FCipher[fhi]);	 /* mark current pos */
 	if (pos == INVALID_SET_FILE_POINTER) {
 		pos = 0;	// ?
 	}
@@ -1458,14 +1542,32 @@ static WORD TTLFileOpen(void)
 	HANDLE FH;
 	int Append, ReadonlyFlag=0;
 	TStrVal FName;
+	TStrVal password;
+	CipherInfoP cip = NULL;
 
 	Err = 0;
 	GetIntVar(&VarId, &Err);
 	GetStrVal(FName, &Err);
 	GetIntVal(&Append, &Err);
-	// get 4nd arg(optional) if given
-	if (CheckParameterGiven()) { // readonly
-		GetIntVal(&ReadonlyFlag, &Err);
+
+	password[0] = 0;
+	while (CheckParameterGiven()) {
+		TStrVal StrTmp;
+		GetStrVal2(StrTmp, &Err, TRUE);
+		if (Err != 0) {
+			Err = ErrSyntax;
+			break;
+		}
+		if (_wcsnicmp(wc::fromUtf8(StrTmp), L"password=", 9) == 0) {
+			wchar_t dummy[MaxStrLen];
+			if (swscanf_s(wc::fromUtf8(StrTmp), L"%[^=]=%hs", dummy, MaxStrLen, password, MaxStrLen) != 2) {
+				Err = ErrSyntax;
+			}
+		} else {
+			if (swscanf_s(wc::fromUtf8(StrTmp), L"%d", &ReadonlyFlag) != 1) {
+				Err = ErrSyntax;
+			}
+		}
 	}
 	if ((Err==0) &&
 	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
@@ -1478,33 +1580,61 @@ static WORD TTLFileOpen(void)
 	}
 
 	wc FNameW = wc::fromUtf8(FName);
-	if (ReadonlyFlag) {
-		FH = CreateFileW(FNameW,
-						 GENERIC_READ, FILE_SHARE_READ, NULL,
-						 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
-	}
-	else {
-		// ファイルをオープンする。
-		// 存在しない場合は作成した後オープンする。
-		// TTL のファイルハンドルは filelock でロックするので、
-		// dwShareMode での共有モードは Read/Write とも有効にする。
-		FH = CreateFileW(FNameW,
-						 GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
-						 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
-	}
-	if (FH == INVALID_HANDLE_VALUE) {
-		SetIntVal(VarId, -1);
-		return Err;
-	}
-	fhi = HandlePut(FH);
-	if (fhi == -1) {
-		SetIntVal(VarId, -1);
-		CloseHandle(FH);
-		return Err;
+	if (password[0] == 0) {
+		if (ReadonlyFlag) {
+			FH = CreateFileW(FNameW,
+							 GENERIC_READ, FILE_SHARE_READ, NULL,
+							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
+		}
+		else {
+			// ファイルをオープンする。
+			// 存在しない場合は作成した後オープンする。
+			// TTL のファイルハンドルは filelock でロックするので、
+			// dwShareMode での共有モードは Read/Write とも有効にする。
+			FH = CreateFileW(FNameW,
+							 GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
+							 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
+		}
+		if (FH == INVALID_HANDLE_VALUE) {
+			SetIntVal(VarId, -1);
+			return Err;
+		}
+		fhi = HandlePut(FH, NULL);
+		if (fhi == -1) {
+			SetIntVal(VarId, -1);
+			CloseHandle(FH);
+			return Err;
+		}
+	} else {
+		if (ReadonlyFlag) {
+			FH = EncryptCreateFileW(FNameW,
+									GENERIC_READ, FILE_SHARE_READ, NULL,
+									OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, password, &cip);
+		}
+		else {
+			// ファイルをオープンする。
+			// 存在しない場合は作成した後オープンする。
+			// TTL のファイルハンドルは filelock でロックするので、
+			// dwShareMode での共有モードは Read/Write とも有効にする。
+			FH = EncryptCreateFileW(FNameW,
+									GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
+									OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL, password, &cip);
+		}
+		if (FH == INVALID_HANDLE_VALUE) {
+			SetIntVal(VarId, -1);
+			return Err;
+		}
+		fhi = HandlePut(FH, cip);
+		if (fhi == -1) {
+			SetIntVal(VarId, -1);
+			EncryptCloseHandle(fhi);
+			CloseHandle(FH);
+			return Err;
+		}
 	}
 	SetIntVal(VarId, fhi);
 	if (Append!=0) {
-		SetFilePointer(FH, 0, NULL, FILE_END);
+		win16_llseek(FH, 0, FILE_END, cip);
 	}
 	return 0;	// no error
 }
@@ -1598,14 +1728,14 @@ static WORD TTLFileReadln(void)
 	EndLine = FALSE;
 	EndFile = TRUE;
 	do {
-		c = win16_lread(FH, &b, 1);
+		c = win16_lread(FH, &b, 1, FCipher[fhi]);
 		if (c>0) EndFile = FALSE;
 		if (c==1) {
 			switch (b) {
 				case 0x0d:
-					c = win16_lread(FH, &b, 1);
+					c = win16_lread(FH, &b, 1, FCipher[fhi]);
 					if ((c==1) && (b!=0x0a))
-						win16_llseek(FH, -1, 1);
+						win16_llseek(FH, -1, 1, FCipher[fhi]);
 					EndLine = TRUE;
 					break;
 				case 0x0a: EndLine = TRUE; break;
@@ -1659,7 +1789,7 @@ static WORD TTLFileRead(void)
 
 	EndFile = FALSE;
 	for (i = 0 ; i < ReadByte ; i++) {
-		c = win16_lread(FH,&b,1);
+		c = win16_lread(FH, &b, 1, FCipher[fhi]);
 		if (c <= 0) {  // EOF
 			EndFile = TRUE;
 			break;
@@ -1758,7 +1888,7 @@ static WORD TTLFileSeek(void)
 	if ((Err==0) && (GetFirstChar()!=0))
 		Err = ErrSyntax;
 	if (Err!=0) return Err;
-	win16_llseek(FH,i,j);
+	win16_llseek(FH, i, j, FCipher[fhi]);
 	return Err;
 }
 
@@ -1775,7 +1905,7 @@ static WORD TTLFileSeekBack(void)
 		Err = ErrSyntax;
 	if (Err!=0) return Err;
 	/* move back to the marked pos */
-	win16_llseek(FH,FPointer[fhi],0);
+	win16_llseek(FH, FPointer[fhi], 0, FCipher[fhi]);
 	return Err;
 }
 
@@ -1896,13 +2026,13 @@ static WORD TTLFileStrSeek(void)
 	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
 		Err = ErrSyntax;
 	if (Err!=0) return Err;
-	pos = win16_llseek(FH,0,1);
+	pos = win16_llseek(FH, 0, 1, FCipher[fhi]);
 	if (pos == INVALID_SET_FILE_POINTER) return Err;
 
 	Len = strlen(Str);
 	i = 0;
 	do {
-		c = win16_lread(FH,&b,1);
+		c = win16_lread(FH, &b, 1, FCipher[fhi]);
 		if (c==1)
 		{
 			if (b==(BYTE)Str[i])
@@ -1918,7 +2048,7 @@ static WORD TTLFileStrSeek(void)
 		SetResult(1);
 	else {
 		SetResult(0);
-		win16_llseek(FH,pos,0);
+		win16_llseek(FH, pos, 0, FCipher[fhi]);
 	}
 	return Err;
 }
@@ -1942,7 +2072,7 @@ static WORD TTLFileStrSeek2(void)
 	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
 		Err = ErrSyntax;
 	if (Err!=0) return Err;
-	pos = win16_llseek(FH,0,1);
+	pos = win16_llseek(FH, 0, 1, FCipher[fhi]);
 	if (pos == INVALID_SET_FILE_POINTER) return Err;
 
 	Len = strlen(Str);
@@ -1950,8 +2080,8 @@ static WORD TTLFileStrSeek2(void)
 	pos2 = pos;
 	do {
 		Last = (pos2<=0);
-		c = win16_lread(FH,&b,1);
-		pos2 = win16_llseek(FH,-2,1);
+		c = win16_lread(FH, &b, 1, FCipher[fhi]);
+		pos2 = win16_llseek(FH, -2, 1, FCipher[fhi]);
 		if (c==1)
 		{
 			if (b==(BYTE)Str[Len-1-i])
@@ -1968,11 +2098,11 @@ static WORD TTLFileStrSeek2(void)
 		// INVALID_SET_FILE_POINTER になるので、
 		// ゼロオフセットになるように調整する。(2008.10.10 yutaka)
 		if (pos2 == INVALID_SET_FILE_POINTER)
-			win16_llseek(FH, 0, 0);
+			win16_llseek(FH, 0, 0, FCipher[fhi]);
 		SetResult(1);
 	} else {
 		SetResult(0);
-		win16_llseek(FH,pos,0);
+		win16_llseek(FH, pos, 0, FCipher[fhi]);
 	}
 	return Err;
 }
@@ -1984,8 +2114,9 @@ static WORD TTLFileTruncate(void)
 	int result = -1;
 	int TruncByte;
 	BOOL r;
-	HANDLE hFile;
+	HANDLE FH;
 	DWORD pos_low;
+	TStrVal password;
 
 	GetStrVal(FName,&Err);
 	if ((Err==0) &&
@@ -2006,28 +2137,64 @@ static WORD TTLFileTruncate(void)
 	}
 	Err = 0;
 
-	// ファイルオープン、存在しない場合は新規作成
-	hFile = CreateFileW(wc::fromUtf8(FName), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
-	if (hFile == INVALID_HANDLE_VALUE) {
-		goto end;
+	password[0] = 0;
+	if (CheckParameterGiven()) {
+		TStrVal StrTmp;
+		GetStrVal(StrTmp, &Err);
+		if (Err == 0) {
+			if (_wcsnicmp(wc::fromUtf8(StrTmp), L"password=", 9) == 0) {
+				wchar_t dummy[MaxStrLen];
+				if (swscanf_s(wc::fromUtf8(StrTmp), L"%[^=]=%hs", dummy, MaxStrLen, password, MaxStrLen) != 2) {
+					Err = ErrSyntax;
+					goto end;
+				}
+			}
+		}
 	}
 
-	// ファイルを指定したサイズにする、
-	// 拡張した場合、拡張部分の内容は未定義
-	pos_low = SetFilePointer(hFile, TruncByte, NULL, FILE_BEGIN );
-	if (pos_low == INVALID_SET_FILE_POINTER) {
-		goto end_close;
-	}
-	r = SetEndOfFile(hFile);
-	if (r == FALSE) {
-		goto end_close;
+	if (password[0] == 0) {
+		// ファイルオープン、存在しない場合は新規作成
+		FH = CreateFileW(wc::fromUtf8(FName), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
+		if (FH == INVALID_HANDLE_VALUE) {
+			goto end;
+		}
+		// ファイルを指定したサイズにする、
+		// 拡張した場合、拡張部分の内容は未定義
+		pos_low = SetFilePointer(FH, TruncByte, NULL, FILE_BEGIN );
+		if (pos_low == INVALID_SET_FILE_POINTER) {
+			goto end_close;
+		}
+		r = SetEndOfFile(FH);
+		if (r == FALSE) {
+			goto end_close;
+		}
+	} else {
+		CipherInfoP cip = NULL;
+		FH = EncryptCreateFileW(wc::fromUtf8(FName), GENERIC_READ|GENERIC_WRITE, 0, NULL,
+								OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL, password, &cip);
+		if (FH == INVALID_HANDLE_VALUE) {
+			goto end;
+		}
+		// 暗号化ファイルの本文を指定したサイズにする。
+		// (ファイルサイズは指定したサイズ＋ENC_FILE_OFFSET)
+		// 拡張した場合、拡張部分の内容はNULLパディング
+		pos_low = win16_llseek(FH, TruncByte, FILE_BEGIN, cip);
+		if (pos_low == INVALID_SET_FILE_POINTER) {
+			EncryptCloseFile(FH, cip);
+ 			goto end_close;
+		}
+		r = SetEndOfFile(FH);
+		if (r == FALSE) {
+			EncryptCloseFile(FH, cip);
+ 			goto end_close;
+		}
 	}
 
 	result = 0;
 	Err = 0;
 
 end_close:
-	CloseHandle( hFile );
+	CloseHandle(FH);
 end:
 	SetResult(result);
 	return Err;
@@ -2046,13 +2213,15 @@ static WORD TTLFileWrite(BOOL addCRLF)
 	FH = HandleGet(fhi);
 	if (Err) return Err;
 
+	FCipher[fhi]->update = TRUE;
+
 	P = LinePtr;
 	GetStrVal(Str, &Err);
 	if (!Err) {
 		if (GetFirstChar())
 			return ErrSyntax;
 
-		win16_lwrite(FH, Str, strlen(Str));
+		win16_lwrite(FH, Str, strlen(Str), FCipher[fhi]);
 	}
 	else if (Err == ErrTypeMismatch) {
 		Err = 0;
@@ -2063,14 +2232,14 @@ static WORD TTLFileWrite(BOOL addCRLF)
 			return ErrSyntax;
 
 		Str[0] = Val & 0xff;
-		win16_lwrite(FH, Str, 1);
+		win16_lwrite(FH, Str, 1, FCipher[fhi]);
 	}
 	else {
 		return Err;
 	}
 
 	if (addCRLF) {
-		win16_lwrite(FH,"\015\012",2);
+		win16_lwrite(FH, "\015\012", 2, FCipher[fhi]);
 	}
 	return 0;
 }

## 追加ファイル ttmencfile.h  
```cpp
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
#define ENC_FILE_OFFSET		16 			// "Salted__"(8文字) ＋ salt(8文字)
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
```

## 追加ファイル ttmencfile.cpp  

```cpp
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
```
