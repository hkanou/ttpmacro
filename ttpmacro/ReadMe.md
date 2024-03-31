# listbox修正内容

- アイコン画像の設定  
- ダブルクリックによる項目選択オプションの追加  
- 最小化/最大化ボタンのオプションの追加  
- 最小化状態での表示オプションの追加  
- 最大化状態での表示オプションの追加  
- ダイアログのサイズを拡大オプションの追加

# 使用方法

(http://htmlpreview.github.io/?https://github.com/hkanou/ttpmacro/blob/main/ttpmacro/doc/listbox.html)

# パッチ

## % diff -rup ttpmacro_org/inpdlg.cpp ttpmacro/inpdlg.cpp
```diff
--- ttpmacro_org/inpdlg.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/inpdlg.cpp	2024-03-31 12:22:33.713520100 +0900
@@ -56,6 +56,7 @@ CInpDlg::CInpDlg(wchar_t *Input, const w
 
 INT_PTR CInpDlg::DoModal(HINSTANCE hInst, HWND hWndParent)
 {
+	m_hInst = hInst;
 	return TTCDialog::DoModal(hInst, hWndParent, CInpDlg::IDD);
 }
 
@@ -68,6 +69,7 @@ BOOL CInpDlg::OnInitDialog()
 	RECT R;
 	HWND HEdit, HOk;
 
+	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), 0);
 	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFile);
 	SetWindowTextW(TitleStr);
 	SetDlgItemTextW(IDC_INPTEXT,TextStr);
@@ -105,6 +107,16 @@ BOOL CInpDlg::OnOK()
 	return TRUE;
 }
 
+BOOL CInpDlg::OnClose()
+{
+	int ret = MessageBoxHaltScript(m_hWnd);
+	if (ret == IDYES) {
+		InputStr = NULL;
+		EndDialog(IDCLOSE);
+	}
+	return TRUE;
+}
+
 LRESULT CInpDlg::OnExitSizeMove(WPARAM wParam, LPARAM lParam)
 {
 	RECT R;
```
## % diff -rup ttpmacro_org/inpdlg.h ttpmacro/inpdlg.h
```diff
--- ttpmacro_org/inpdlg.h	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/inpdlg.h	2024-03-31 12:22:33.713714100 +0900
@@ -52,9 +52,11 @@ private:
 	BOOL PaswdFlag;
 	int init_WW, TW, TH, BH, BW, EW, EH;
 	SIZE s;
+	HINSTANCE m_hInst;
 
 	virtual BOOL OnInitDialog();
 	virtual BOOL OnOK();
+	virtual BOOL OnClose();
 	virtual LRESULT DlgProc(UINT msg, WPARAM wp, LPARAM lp);
 	LRESULT OnExitSizeMove(WPARAM wp, LPARAM lp);
  	void Relocation(BOOL is_init, int WW);
```
## % diff -rup ttpmacro_org/ListDlg.cpp ttpmacro/ListDlg.cpp
```diff
--- ttpmacro_org/ListDlg.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/ListDlg.cpp	2024-03-31 12:22:33.713940600 +0900
@@ -38,12 +38,16 @@
 #include "dlglib.h"
 #include "ttmdlg.h"
 #include "ttmacro.h"
+#include "ttl.h"
+#include "ttmparse.h"
 
 #include "ListDlg.h"
 
+#define CONTROL_GAP_W	14		// ウィンドウ端とコントロール間との幅
+
 // CListDlg ダイアログ
 
-CListDlg::CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y)
+CListDlg::CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y, int ext, int width, int height)
 {
 	m_Text = Text;
 	m_Caption = Caption;
@@ -51,10 +55,14 @@ CListDlg::CListDlg(const wchar_t *Text,
 	m_Selected = Selected;
 	PosX = x;
 	PosY = y;
+	m_ext = ext;
+	m_width = width;
+	m_height = height;
 }
 
 INT_PTR CListDlg::DoModal(HINSTANCE hInst, HWND hWndParent)
 {
+	m_hInst = hInst;
 	return TTCDialog::DoModal(hInst, hWndParent, IDD);
 }
 
@@ -104,14 +112,19 @@ BOOL CListDlg::OnInitDialog()
 	};
 	RECT R;
 	HWND HList, HOk;
-
-	ResizeHelper = ReiseHelperInit(m_hWnd, resize_info, _countof(resize_info));
+	int NonClientAreaWidth;
+	int NonClientAreaHeight;
 
 	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFile);
 
 	HList = ::GetDlgItem(m_hWnd, IDC_LISTBOX);
 	InitList(HList);
 
+	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), 0);
+	if (m_ext & ExtListBoxMinmaxbutton) {
+		ModifyStyle(0, WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
+	}
+
 	// 本文とタイトル
 	SetDlgItemTextW(IDC_LISTTEXT, m_Text);
 	SetWindowTextW(m_Caption);
@@ -120,10 +133,6 @@ BOOL CListDlg::OnInitDialog()
 	TW = s.cx + s.cx/10;
 	TH = s.cy;
 
-	::GetWindowRect(HList,&R);
-	LW = R.right-R.left;
-	LH = R.bottom-R.top;
-
 	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
 	::GetWindowRect(HOk,&R);
 	BW = R.right-R.left;
@@ -133,7 +142,33 @@ BOOL CListDlg::OnInitDialog()
 	WW = R.right-R.left;
 	WH = R.bottom-R.top;
 
+	if (m_ext & ExtListBoxSize) {
+		LW = TH * m_width / 2;
+		LH = TH * m_height;
+		TW = LW;
+		s.cx = s.cx + s.cx/10;
+		::GetClientRect(m_hWnd, &R);
+		NonClientAreaWidth = WW - (R.right - R.left);
+		NonClientAreaHeight = WH - (R.bottom - R.top);
+		WW = TW + NonClientAreaWidth + CONTROL_GAP_W * 4 + BW;
+		WH = TH + LH + (int)(BH*1.5) + NonClientAreaHeight;
+		SetWindowPos(HWND_TOP, 0, 0, WW, WH, SWP_NOMOVE);
+	} else {
+		::GetWindowRect(HList,&R);
+		LW = R.right-R.left;
+		LH = R.bottom-R.top;
+	}
+
+	ResizeHelper = ReiseHelperInit(m_hWnd, resize_info, _countof(resize_info));
+
 	Relocation(TRUE, WW);
+
+	if (m_ext & ExtListBoxMinimize) {
+		ShowWindow(SW_MINIMIZE);
+	} else if (m_ext & ExtListBoxMaximize) {
+		ShowWindow(SW_MAXIMIZE);
+	}
+
 	BringupWindow(m_hWnd);
 
 	return TRUE;
@@ -173,7 +208,6 @@ void CListDlg::Relocation(BOOL is_init,
 	NonClientAreaWidth = WW - CW;
 	NonClientAreaHeight = WH - CH;
 
-#define CONTROL_GAP_W	14		// ウィンドウ端とコントロール間との幅
 	// 初回のみ
 	if (is_init) {
 		// テキストコントロールサイズを補正
@@ -214,15 +248,34 @@ LRESULT CListDlg::DlgProc(UINT msg, WPAR
 {
 	switch (msg) {
 		case WM_SIZE:
-			ReiseDlgHelper_WM_SIZE(ResizeHelper);
+			if (wp == SIZE_MINIMIZED) {
+				TTLShowMINIMIZE();
+				ShowWindow(SW_MINIMIZE);
+				return TRUE;
+			}
+			if (ResizeHelper != NULL) {
+				ReiseDlgHelper_WM_SIZE(ResizeHelper);
+			}
 			break;
 		case WM_GETMINMAXINFO:
-			ReiseDlgHelper_WM_GETMINMAXINFO(ResizeHelper, lp);
+			if (ResizeHelper != NULL) {
+				ReiseDlgHelper_WM_GETMINMAXINFO(ResizeHelper, lp);
+			}
 			break;
 		case WM_DESTROY:
-			ReiseDlgHelperDelete(ResizeHelper);
-			ResizeHelper = NULL;
+			if (ResizeHelper != NULL) {
+				ReiseDlgHelperDelete(ResizeHelper);
+				ResizeHelper = NULL;
+			}
 			break;
+		case WM_COMMAND:
+			if (m_ext & ExtListBoxDoubleclick) {
+				if (HIWORD(wp) == LBN_DBLCLK) {
+					OnOK();
+					return TRUE;
+				}
+				break;
+			}
 	}
 	return (LRESULT)FALSE;
 }
```
## % diff -rup ttpmacro_org/ListDlg.h ttpmacro/ListDlg.h
```diff
--- ttpmacro_org/ListDlg.h	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/ListDlg.h	2024-03-31 12:22:33.714109900 +0900
@@ -35,7 +35,7 @@
 class CListDlg : public CMacroDlgBase
 {
 public:
-	CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y);
+	CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y, int ext, int width, int height);
 	INT_PTR DoModal(HINSTANCE hInst, HWND hWndParent);
 	int m_SelectItem;
 
@@ -47,7 +47,11 @@ private:
 	int m_Selected;
 	int init_WW, TW, TH, BH, BW, LW, LH;
 	SIZE s;
-	ReiseDlgHelper_t *ResizeHelper;
+	ReiseDlgHelper_t *ResizeHelper = NULL;
+	HINSTANCE m_hInst;
+	int m_ext = 0;
+	int m_width;
+	int m_height;
 
 	void Relocation(BOOL is_init, int WW);
 	void InitList(HWND HList);
```
## % diff -rup ttpmacro_org/msgdlg.cpp ttpmacro/msgdlg.cpp
```diff
--- ttpmacro_org/msgdlg.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/msgdlg.cpp	2024-03-31 12:22:33.714268800 +0900
@@ -57,6 +57,7 @@ CMsgDlg::CMsgDlg(const wchar_t *Text, co
 
 INT_PTR CMsgDlg::DoModal(HINSTANCE hInst, HWND hWndParent)
 {
+	m_hInst = hInst;
 	return TTCDialog::DoModal(hInst, hWndParent, CMsgDlg::IDD);
 }
 
@@ -80,6 +81,7 @@ BOOL CMsgDlg::OnInitDialog()
 		};
 		SetDlgTexts(m_hWnd, TextInfosOk, _countof(TextInfosOk), UILanguageFile);
 	}
+	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), 0);
 
 	SetWindowTextW(TitleStr);
 	SetDlgItemTextW(IDC_MSGTEXT,TextStr);
```
## % diff -rup ttpmacro_org/msgdlg.h ttpmacro/msgdlg.h
```diff
--- ttpmacro_org/msgdlg.h	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/msgdlg.h	2024-03-31 12:22:33.714418500 +0900
@@ -47,6 +47,7 @@ private:
 	BOOL YesNoFlag;
 	int  init_WW, TW, TH, BH, BW;
 	SIZE s;
+	HINSTANCE m_hInst;
 
 	virtual BOOL OnInitDialog();
 	virtual BOOL OnClose();
```
## % diff -rup ttpmacro_org/statdlg.cpp ttpmacro/statdlg.cpp
```diff
--- ttpmacro_org/statdlg.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/statdlg.cpp	2024-03-31 12:22:33.714564400 +0900
@@ -38,6 +38,7 @@
 #include "tmfc.h"
 #include "tttypes.h"
 #include "ttmacro.h"
+#include "dlglib.h"
 
 #include "statdlg.h"
 
@@ -49,6 +50,7 @@ BOOL CStatDlg::Create(HINSTANCE hInst, c
 	TitleStr = Title;
 	PosX = x;
 	PosY = y;
+	m_hInst = hInst;
 	return TTCDialog::Create(hInst, NULL, CStatDlg::IDD);
 }
 
@@ -91,6 +93,9 @@ void CStatDlg::Update(const wchar_t *Tex
 
 BOOL CStatDlg::OnInitDialog()
 {
+	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), 0);
+	// 閉じるボタンを無効化
+	RemoveMenu(GetSystemMenu(m_hWnd, FALSE), SC_CLOSE, MF_BYCOMMAND);
 
 	Update(TextStr,TitleStr,PosX,PosY);
 	SetForegroundWindow(m_hWnd);
```
## % diff -rup ttpmacro_org/statdlg.h ttpmacro/statdlg.h
```diff
--- ttpmacro_org/statdlg.h	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/statdlg.h	2024-03-31 12:22:33.714722900 +0900
@@ -43,6 +43,7 @@ private:
 	const wchar_t *TitleStr;
 	int  init_WW, TW, TH;
 	SIZE s;
+	HINSTANCE m_hInst;
 
 	virtual BOOL OnInitDialog();
 	virtual BOOL OnOK();
```
## % diff -rup ttpmacro_org/ttl.cpp ttpmacro/ttl.cpp
```diff
--- ttpmacro_org/ttl.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/ttl.cpp	2024-03-31 12:22:33.714882600 +0900
@@ -4144,6 +4144,11 @@ static WORD TTLShow(void)
 	return Err;
 }
 
+void TTLShowMINIMIZE(void)
+{
+	ShowWindow(HMainWin,SW_MINIMIZE);
+}
+
 // 'sprintf': Format a string in the style of sprintf
 //
 // (2007.5.1 yutaka)
```
## % diff -rup ttpmacro_org/ttl.h ttpmacro/ttl.h
```diff
--- ttpmacro_org/ttl.h	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/ttl.h	2024-03-31 12:22:33.715030800 +0900
@@ -54,6 +54,7 @@ void SetResult(int ResultCode);
 BOOL CheckTimeout();
 BOOL TestWakeup(int Wakeup);
 void SetWakeup(int Wakeup);
+void TTLShowMINIMIZE();
 
 // exit code of TTMACRO
 extern int ExitCode;
```
## % diff -rup ttpmacro_org/ttl_gui.cpp ttpmacro/ttl_gui.cpp
```diff
--- ttpmacro_org/ttl_gui.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/ttl_gui.cpp	2024-03-31 12:22:33.715177800 +0900
@@ -214,18 +214,25 @@ WORD TTLGetPassword()
 	GetAbsPath(Str,sizeof(Str));
 
 	GetPrivateProfileStringW(L"Password", (wc)Str2, L"",
-	                         Temp, _countof(Temp), (wc)Str);
+                             Temp, _countof(Temp), (wc)Str);
 	if (Temp[0]==0) // password not exist
 	{
 		wchar_t input_string[MaxStrLen] = {};
 		size_t Temp2_len = sizeof(Temp2);
-		OpenInpDlg(input_string, wc::fromUtf8(Str2), L"Enter password", L"", TRUE);
-		WideCharToUTF8(input_string, NULL, Temp2, &Temp2_len);
-		if (Temp2[0]!=0) {
-			char TempA[512];
-			Encrypt(Temp2, TempA);
-			if (WritePrivateProfileStringW(L"Password", (wc)Str2, (wc)TempA, wc::fromUtf8(Str)) != 0) {
-				result = 1;  /* success */
+		Err = OpenInpDlg(input_string, wc::fromUtf8(Str2), L"Enter password", L"", TRUE);
+		if (Err == IDCLOSE) {
+			// リストボックスの閉じるボタン(&確認ダイアログ)で、マクロの終了とする。
+			TTLStatus = IdTTLEnd;
+			return 0;
+		} else {
+			Err = 0;
+			WideCharToUTF8(input_string, NULL, Temp2, &Temp2_len);
+			if (Temp2[0]!=0) {
+				char TempA[512];
+				Encrypt(Temp2, TempA);
+				if (WritePrivateProfileStringW(L"Password", (wc)Str2, (wc)TempA, wc::fromUtf8(Str)) != 0) {
+					result = 1;  /* success */
+				}
 			}
 		}
 	}
@@ -286,10 +293,17 @@ WORD TTLInputBox(BOOL Paswd)
 	SetInputStr("");
 	if (CheckVar("inputstr",&ValType,&VarId) && (ValType==TypString)) {
 		wchar_t input_string[MaxStrLen];
-		OpenInpDlg(input_string,wc::fromUtf8(Str1),wc::fromUtf8(Str2),wc::fromUtf8(Str3),Paswd);
-		char *u8 = ToU8W(input_string);
-		SetStrVal(VarId, u8);
-		free(u8);
+		Err = OpenInpDlg(input_string,wc::fromUtf8(Str1),wc::fromUtf8(Str2),wc::fromUtf8(Str3),Paswd);
+		if (Err == IDCLOSE) {
+			// リストボックスの閉じるボタン(&確認ダイアログ)で、マクロの終了とする。
+		  	TTLStatus = IdTTLEnd;
+			SetStrVal(VarId, "");
+		} else {
+			char *u8 = ToU8W(input_string);
+			SetStrVal(VarId, u8);
+			free(u8);
+		}
+		Err = 0;
 	}
 	return Err;
 }
@@ -351,6 +365,10 @@ static int MessageCommand(MessageCommand
 	int i, ary_size;
 	int sel = 0;
 	TVarId VarId;
+	TStrVal StrTmp;
+	int ext = 0;
+	int width;
+	int height;
 
 	*Err = 0;
 	GetStrVal2(Str1, Err, TRUE);
@@ -393,13 +411,44 @@ static int MessageCommand(MessageCommand
 	} else if (BoxId==IdListBox) {
 		//  リストボックスの選択肢を取得する。
 		GetStrAryVar(&VarId, Err);
-
-		if (CheckParameterGiven()) {
-			GetIntVal(&sel, Err);
+		while (CheckParameterGiven()) {
+			GetStrVal2(StrTmp, Err, TRUE);
+			if (*Err == 0) {
+				if (_wcsicmp(wc::fromUtf8(StrTmp), L"dclick=on") == 0) {
+					ext |= ExtListBoxDoubleclick;
+				} else if (_wcsicmp(wc::fromUtf8(StrTmp), L"minmaxbutton=on") == 0) {
+					ext |= ExtListBoxMinmaxbutton;
+				} else if (_wcsicmp(wc::fromUtf8(StrTmp), L"minimize=on") == 0) {
+					ext |=  ExtListBoxMinimize;
+					ext &= ~ExtListBoxMaximize;
+				} else if (_wcsicmp(wc::fromUtf8(StrTmp), L"maximize=on") == 0) {
+					ext &= ~ExtListBoxMinimize;
+					ext |=  ExtListBoxMaximize;
+				} else if (_wcsnicmp(wc::fromUtf8(StrTmp), L"size=", 5) == 0) {
+				  	wchar_t dummy1[24], dummy2[24];
+					if (swscanf_s(wc::fromUtf8(StrTmp), L"%[^=]=%d%[xX]%d", dummy1, 24, &width, dummy2, 24, &height) == 4) {
+						if (width < 0 || height < 0) {
+							*Err = ErrSyntax;
+							break;
+						} else {
+							ext |= ExtListBoxSize;
+						}
+					} else {
+						*Err = ErrSyntax;
+						break;
+					}
+				} else if (sscanf_s(StrTmp, "%d", &sel) != 1) {
+					*Err = ErrSyntax;
+					break;
+				}
+			} else {
+				break;
+			}
 		}
-		if (*Err==0 && GetFirstChar()!=0)
+		if (*Err == 0 && GetFirstChar() != 0) {
 			*Err = ErrSyntax;
-		if (*Err!=0) return 0;
+		}
+		if (*Err != 0) return 0;
 
 		ary_size = GetStrAryVarSize(VarId);
 		if (sel < 0 || sel >= ary_size) {
@@ -426,7 +475,7 @@ static int MessageCommand(MessageCommand
 		//   0以上: 選択項目
 		//   -1: キャンセル
 		//	 -2: close
-		ret = OpenListDlg(wc::fromUtf8(Str1), wc::fromUtf8(Str2), s, sel);
+		ret = OpenListDlg(wc::fromUtf8(Str1), wc::fromUtf8(Str2), s, sel, ext, width, height);
 
 		for (i = 0 ; i < ary_size ; i++) {
 			free((void *)s[i]);
```
## % diff -rup ttpmacro_org/ttmdlg.cpp ttpmacro/ttmdlg.cpp
```diff
--- ttpmacro_org/ttmdlg.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/ttmdlg.cpp	2024-03-31 12:22:33.715324100 +0900
@@ -220,13 +220,13 @@ void SetDlgPos(int x, int y)
 	}
 }
 
-void OpenInpDlg(wchar_t *Buff, const wchar_t *Text, const wchar_t *Caption,
+int OpenInpDlg(wchar_t *Buff, const wchar_t *Text, const wchar_t *Caption,
                 const wchar_t *Default, BOOL Paswd)
 {
 	HINSTANCE hInst = GetInstance();
 	HWND hWndParent = GetHWND();
 	CInpDlg InpDlg(Buff,Text,Caption,Default,Paswd,DlgPosX,DlgPosY);
-	InpDlg.DoModal(hInst, hWndParent);
+	return InpDlg.DoModal(hInst, hWndParent);
 }
 
 int OpenErrDlg(const char *Msg, const char *Line, int lineno, int start, int end, const char *FileName)
@@ -282,11 +282,11 @@ void BringupStatDlg()
  * @retval -1		cancelボタン
  * @retval -2		closeボタン
  */
-int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected)
+int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int ext, int DlgWidth, int DlgHeight)
 {
 	HINSTANCE hInst = GetInstance();
 	HWND hWndParent = GetHWND();
-	CListDlg ListDlg(Text, Caption, Lists, Selected, DlgPosX, DlgPosY);
+	CListDlg ListDlg(Text, Caption, Lists, Selected, DlgPosX, DlgPosY, ext, DlgWidth, DlgHeight);
 	INT_PTR r = ListDlg.DoModal(hInst, hWndParent);
 	if (r == IDOK) {
 		return ListDlg.m_SelectItem;
```
## % diff -rup ttpmacro_org/ttmdlg.h ttpmacro/ttmdlg.h
```diff
--- ttpmacro_org/ttmdlg.h	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/ttmdlg.h	2024-03-31 12:22:33.715470600 +0900
@@ -40,7 +40,7 @@ extern "C" {
 void ParseParam(PBOOL IOption, PBOOL VOption);
 BOOL GetFileName(HWND HWin);
 void SetDlgPos(int x, int y);
-void OpenInpDlg(wchar_t *Input, const wchar_t *Text, const wchar_t *Caption,
+int OpenInpDlg(wchar_t *Input, const wchar_t *Text, const wchar_t *Caption,
                 const wchar_t *Default, BOOL Paswd);
 int OpenErrDlg(const char *Msg, const char *Line, int lineno, int start, int end, const char *FileName);
 int OpenMsgDlg(const wchar_t *Text, const wchar_t *Caption, BOOL YesNo);
@@ -48,7 +48,7 @@ void OpenStatDlg(const wchar_t *Text, co
 void CloseStatDlg();
 void BringupStatDlg();
 
-int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected);
+int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int ext, int DlgWidth, int DlgHeight);
 
 extern wchar_t *HomeDirW;
 extern wchar_t FileName[MAX_PATH];
```
## % diff -rup ttpmacro_org/ttmparse.h ttpmacro/ttmparse.h
```diff
--- ttpmacro_org/ttmparse.h	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/ttmparse.h	2024-03-31 12:22:33.715618900 +0900
@@ -306,6 +306,11 @@
 #define RsvALShift      1021 // arithmetic left shift
 #define RsvLRShift      1022 // logical right shift
 
+#define ExtListBoxDoubleclick   1
+#define ExtListBoxMinmaxbutton  2
+#define ExtListBoxMinimize      4
+#define ExtListBoxMaximize      8
+#define ExtListBoxSize          16
 
 // integer type for buffer pointer
 typedef DWORD BINT;
```
## % diff -rup ttpmacro_org/ttpmacro.rc ttpmacro/ttpmacro.rc
```diff
--- ttpmacro_org/ttpmacro.rc	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro/ttpmacro.rc	2024-03-31 12:22:33.715773900 +0900
@@ -75,7 +75,7 @@ BEGIN
 END
 
 IDD_INPDLG DIALOGEX 20, 20, 143, 59
-STYLE DS_SETFONT | DS_3DLOOK | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_THICKFRAME
+STYLE DS_SETFONT | DS_3DLOOK | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME
 FONT 14, "System", 0, 0, 0x0
 BEGIN
     LTEXT           "",IDC_INPTEXT,8,8,126,10,SS_NOPREFIX
@@ -93,7 +93,7 @@ BEGIN
 END
 
 IDD_STATDLG DIALOGEX 0, 0, 52, 25
-STYLE DS_SETFONT | DS_3DLOOK | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_THICKFRAME
+STYLE DS_SETFONT | DS_3DLOOK | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME
 FONT 14, "System", 0, 0, 0x0
 BEGIN
     LTEXT           "ABC",IDC_STATTEXT,0,7,50,9,SS_NOPREFIX
@@ -101,6 +101,7 @@ END
 
 IDD_LISTDLG DIALOGEX 0, 0, 186, 86
 STYLE DS_SETFONT | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME
+EXSTYLE WS_EX_APPWINDOW
 CAPTION "Dialog"
 FONT 14, "System", 0, 0, 0x0
 BEGIN
```
