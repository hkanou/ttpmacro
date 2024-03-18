# listbox修正内容(その２)

- TTLコマンドlistboxへのオプション追加
```
  修正前 listbox <message> <title> <string array> [<selected>]
  修正後 listbox <message> <title> <string array> [<selected> ['doubleclick'] ['minmaxbutton'] ['size=large']]
  例) listbox '今日の天気はどうですか?' 'あなたへの問い' msg 6 'doubleclick' 'minmaxbutton' 'size=large'
  ※ size=medium、size=hugeは、要望があれば追加
```
- アイコン画像の差し替え  
- 最小化/最大化ボタンの追加  
- ダブルクリックによる項目選択に対応  
- ダイアログのサイズを拡大

## % diff -up org/ttl_gui.cpp ttl_gui.cpp
```diff
--- org/ttl_gui.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttl_gui.cpp	2024-03-19 01:27:58.708673500 +0900
@@ -351,6 +351,10 @@ static int MessageCommand(MessageCommand
 	int i, ary_size;
 	int sel = 0;
 	TVarId VarId;
+	TStrVal StrTmp;
+	BOOL doubleclick  = FALSE;
+	BOOL minmaxbutton = FALSE;
+	INT  dlg_size = 0;
 
 	*Err = 0;
 	GetStrVal2(Str1, Err, TRUE);
@@ -397,10 +401,30 @@ static int MessageCommand(MessageCommand
 		if (CheckParameterGiven()) {
 			GetIntVal(&sel, Err);
 		}
-		if (*Err==0 && GetFirstChar()!=0)
-			*Err = ErrSyntax;
 		if (*Err!=0) return 0;
 
+		while (CheckParameterGiven()) {
+			GetStrVal2(StrTmp, Err, TRUE);
+			if (*Err == 0) {
+				if (_wcsicmp(wc::fromUtf8(StrTmp), L"doubleclick") == 0) {
+					doubleclick = TRUE;
+				} else if (_wcsicmp(wc::fromUtf8(StrTmp), L"minmaxbutton") == 0) {
+					minmaxbutton = TRUE;
+				} else if (_wcsicmp(wc::fromUtf8(StrTmp), L"size=large") == 0) {
+					dlg_size = 1;
+				} else {
+					*Err = ErrSyntax;
+					break;
+				}
+			} else {
+				break;
+			}
+		}
+		if (*Err == 0 && GetFirstChar() != 0) {
+			*Err = ErrSyntax;
+		}
+		if (*Err != 0) return 0;
+
 		ary_size = GetStrAryVarSize(VarId);
 		if (sel < 0 || sel >= ary_size) {
 			sel = 0;
@@ -426,7 +450,7 @@ static int MessageCommand(MessageCommand
 		//   0以上: 選択項目
 		//   -1: キャンセル
 		//	 -2: close
-		ret = OpenListDlg(wc::fromUtf8(Str1), wc::fromUtf8(Str2), s, sel);
+		ret = OpenListDlg(wc::fromUtf8(Str1), wc::fromUtf8(Str2), s, sel, doubleclick, minmaxbutton, dlg_size);
 
 		for (i = 0 ; i < ary_size ; i++) {
 			free((void *)s[i]);
```

## % diff -up org/ttmdlg.h ttmdlg.h
```diff
--- org/ttmdlg.h	2024-02-28 09:02:00.000000000 +0900
+++ ttmdlg.h	2024-03-19 01:27:58.709108000 +0900
@@ -48,7 +48,7 @@ void OpenStatDlg(const wchar_t *Text, co
 void CloseStatDlg();
 void BringupStatDlg();
 
-int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected);
+int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, BOOL doubleclick, BOOL minmaxbutton, INT dlg_size);
 
 extern wchar_t *HomeDirW;
 extern wchar_t FileName[MAX_PATH];
```

## % diff -up org/ttmdlg.cpp ttmdlg.cpp
```diff
--- org/ttmdlg.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ttmdlg.cpp	2024-03-19 01:27:58.708966400 +0900
@@ -282,11 +282,11 @@ void BringupStatDlg()
  * @retval -1		cancelボタン
  * @retval -2		closeボタン
  */
-int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected)
+int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, BOOL doubleclick, BOOL minmaxbutton, INT dlg_size)
 {
 	HINSTANCE hInst = GetInstance();
 	HWND hWndParent = GetHWND();
-	CListDlg ListDlg(Text, Caption, Lists, Selected, DlgPosX, DlgPosY);
+	CListDlg ListDlg(Text, Caption, Lists, Selected, DlgPosX, DlgPosY, doubleclick, minmaxbutton, dlg_size);
 	INT_PTR r = ListDlg.DoModal(hInst, hWndParent);
 	if (r == IDOK) {
 		return ListDlg.m_SelectItem;
```

## % diff -up org/ListDlg.h ListDlg.h
```diff
--- org/ListDlg.h	2024-02-28 09:02:00.000000000 +0900
+++ ListDlg.h	2024-03-19 01:27:58.708141400 +0900
@@ -35,7 +35,7 @@
 class CListDlg : public CMacroDlgBase
 {
 public:
-	CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y);
+	CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y, BOOL doubleclick, BOOL minmaxbutton, INT dlg_size);
 	INT_PTR DoModal(HINSTANCE hInst, HWND hWndParent);
 	int m_SelectItem;
 
@@ -48,6 +48,11 @@ private:
 	int init_WW, TW, TH, BH, BW, LW, LH;
 	SIZE s;
 	ReiseDlgHelper_t *ResizeHelper;
+	HINSTANCE hInstList;
+	BOOL minimized = FALSE;
+	BOOL m_doubleclick = FALSE;
+	BOOL m_minmaxbutton = FALSE;
+	INT m_dlg_size = 0; // 1:large
 
 	void Relocation(BOOL is_init, int WW);
 	void InitList(HWND HList);
```

## % diff -up org/ListDlg.cpp ListDlg.cpp
```diff
--- org/ListDlg.cpp	2024-02-28 09:02:00.000000000 +0900
+++ ListDlg.cpp	2024-03-19 01:27:58.707956100 +0900
@@ -43,7 +43,7 @@
 
 // CListDlg ダイアログ
 
-CListDlg::CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y)
+CListDlg::CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y, BOOL doubleclick, BOOL minmaxbutton, INT dlg_size)
 {
 	m_Text = Text;
 	m_Caption = Caption;
@@ -51,10 +51,18 @@ CListDlg::CListDlg(const wchar_t *Text,
 	m_Selected = Selected;
 	PosX = x;
 	PosY = y;
+	m_doubleclick = doubleclick;
+	m_minmaxbutton = minmaxbutton;
+	m_dlg_size = dlg_size;
 }
 
 INT_PTR CListDlg::DoModal(HINSTANCE hInst, HWND hWndParent)
 {
+	hInstList = hInst;
+	if (m_dlg_size == 1) {
+		// ラージサイズの場合は、リソースを差し替え
+		return TTCDialog::DoModal(hInst, hWndParent, IDD_LISTDLG_LARGE);
+	}
 	return TTCDialog::DoModal(hInst, hWndParent, IDD);
 }
 
@@ -112,6 +120,13 @@ BOOL CListDlg::OnInitDialog()
 	HList = ::GetDlgItem(m_hWnd, IDC_LISTBOX);
 	InitList(HList);
 
+	// アイコンを設定
+	TTSetIcon(hInstList, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO_FILE), 0);
+	if(m_minmaxbutton == TRUE) {
+		// 最小、最大化ボタンを追加
+		ModifyStyle(0, WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
+	}
+
 	// 本文とタイトル
 	SetDlgItemTextW(IDC_LISTTEXT, m_Text);
 	SetWindowTextW(m_Caption);
@@ -214,6 +229,18 @@ LRESULT CListDlg::DlgProc(UINT msg, WPAR
 {
 	switch (msg) {
 		case WM_SIZE:
+			// 最小化の場合の挙動を変更
+			if (wp == SIZE_MINIMIZED) {
+				minimized = TRUE;
+				return TRUE;
+			}
+			if (wp == SIZE_MAXIMIZED) {
+				if (minimized == TRUE) {
+					minimized = FALSE;
+					ShowWindow(SW_RESTORE);
+					return TRUE;
+				}
+			}
 			ReiseDlgHelper_WM_SIZE(ResizeHelper);
 			break;
 		case WM_GETMINMAXINFO:
@@ -223,6 +250,15 @@ LRESULT CListDlg::DlgProc(UINT msg, WPAR
 			ReiseDlgHelperDelete(ResizeHelper);
 			ResizeHelper = NULL;
 			break;
+		case WM_COMMAND:
+			// ダブルクリックを有効化
+			if (m_doubleclick == TRUE) {
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

## % diff -up org/ttm_res.h ttm_res.h
```diff
--- org/ttm_res.h	2024-02-28 09:02:00.000000000 +0900
+++ ttm_res.h	2024-03-19 01:27:58.708822000 +0900
@@ -26,6 +26,7 @@
 #define IDC_FILENAME                    606
 #define IDI_TTMACRO_FLAT                607
 #define IDI_TTMACRO_FILE                608
+#define IDD_LISTDLG_LARGE               609
 
 // Next default values for new objects
 // 
```

## % diff -up org/ttpmacro.rc ttpmacro.rc
```diff
--- org/ttpmacro.rc	2024-02-28 09:02:00.000000000 +0900
+++ ttpmacro.rc	2024-03-19 01:27:58.709263400 +0900
@@ -110,6 +110,17 @@ BEGIN
     LTEXT           "Static",IDC_LISTTEXT,7,63,172,15
 END
 
+// IDD_LISTDLG ラージ版
+IDD_LISTDLG_LARGE DIALOGEX 0, 0, 362, 223
+STYLE DS_SETFONT | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME
+CAPTION "Dialog"
+FONT 14, "System", 0, 0, 0x0
+BEGIN
+    DEFPUSHBUTTON   "OK",IDOK,300,7,30,14
+    PUSHBUTTON      "Cancel",IDCANCEL,300,24,30,14
+    LISTBOX         IDC_LISTBOX,4,7,266,196,LBS_USETABSTOPS | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP
+    LTEXT           "Static",IDC_LISTTEXT,4,207,172,15
+END
 
 /////////////////////////////////////////////////////////////////////////////
 //
@@ -136,6 +147,14 @@ BEGIN
         TOPMARGIN, 7
         BOTTOMMARGIN, 79
     END
+
+    IDD_LISTDLG_LARGE, DIALOG
+    BEGIN
+        LEFTMARGIN, 7
+        RIGHTMARGIN, 179
+        TOPMARGIN, 7
+        BOTTOMMARGIN, 79
+    END
 END
 #endif    // APSTUDIO_INVOKED
 
@@ -172,6 +191,10 @@ BEGIN
     0
 END
 
+IDD_LISTDLG_LARGE AFX_DIALOG_LAYOUT
+BEGIN
+    0
+END
 #endif    // 英語 (米国) resources
 /////////////////////////////////////////////////////////////////////////////
 
```
