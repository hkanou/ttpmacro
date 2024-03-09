# ����

## % diff -up _org_ListDlg.cpp ListDlg.cpp
```diff
--- _org_ListDlg.cpp	2024-02-29 02:02:00.000000000 +0900
+++ ListDlg.cpp	2024-03-09 22:42:21.271058100 +0900
@@ -55,6 +55,8 @@ CListDlg::CListDlg(const wchar_t *Text,
 
 INT_PTR CListDlg::DoModal(HINSTANCE hInst, HWND hWndParent)
 {
+	// HINSTANCE��ۑ�
+	hInstList = hInst;
 	return TTCDialog::DoModal(hInst, hWndParent, IDD);
 }
 
@@ -112,12 +114,16 @@ BOOL CListDlg::OnInitDialog()
 	HList = ::GetDlgItem(m_hWnd, IDC_LISTBOX);
 	InitList(HList);
 
+	// ���[�_���_�C�A���O�ɃA�C�R����ݒ� (2024/3/9)
+	TTSetIcon(hInstList, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO_FILE), 0);
+
 	// �{���ƃ^�C�g��
 	SetDlgItemTextW(IDC_LISTTEXT, m_Text);
 	SetWindowTextW(m_Caption);
 
 	CalcTextExtentW(GetDlgItem(IDC_LISTTEXT), NULL, m_Text,&s);
-	TW = s.cx + s.cx/10;
+	// �����t�H���g�p��TW���C�� (2024/3/9)
+	TW = s.cx/2 + s.cx/10;
 	TH = s.cy;
 
 	::GetWindowRect(HList,&R);
@@ -214,6 +220,18 @@ LRESULT CListDlg::DlgProc(UINT msg, WPAR
 {
 	switch (msg) {
 		case WM_SIZE:
+			// �ŏ����̏ꍇ�̋�����ύX (2024/3/9)
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
@@ -223,6 +241,13 @@ LRESULT CListDlg::DlgProc(UINT msg, WPAR
 			ReiseDlgHelperDelete(ResizeHelper);
 			ResizeHelper = NULL;
 			break;
+		case WM_COMMAND:
+			// �_�u���N���b�N��L���� (2024/3/9)
+			if (HIWORD(wp) == LBN_DBLCLK) {
+				OnOK();
+				return TRUE;
+			}
+			break;
 	}
 	return (LRESULT)FALSE;
 }
```

## % diff -up _org_ListDlg.h ListDlg.h
```diff
--- _org_ListDlg.h      2024-03-09 18:15:42.917142300 +0900
+++ ListDlg.h   2024-03-09 20:55:58.724749400 +0900
@@ -48,6 +48,9 @@ private:
        int init_WW, TW, TH, BH, BW, LW, LH;
        SIZE s;
        ReiseDlgHelper_t *ResizeHelper;
+       // �ǉ� (2024/3/9)
+       HINSTANCE hInstList;
+       BOOL minimized = FALSE;

        void Relocation(BOOL is_init, int WW);
        void InitList(HWND HList);
```

## % diff -up _org_ttpmacro.rc ttpmacro.rc
```diff
--- _org_ttpmacro.rc    2024-03-09 13:52:28.764303000 +0900
+++ ttpmacro.rc 2024-03-09 20:56:04.552887500 +0900
@@ -99,15 +99,16 @@ BEGIN
     LTEXT           "ABC",IDC_STATTEXT,0,7,50,9,SS_NOPREFIX
 END

-IDD_LISTDLG DIALOGEX 0, 0, 186, 86
-STYLE DS_SETFONT | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME
+// �ŏ���/�ő剻�{�^���ǉ��A�T�C�Y�ύX (2024/3/9)
+IDD_LISTDLG DIALOGEX 0, 0, 362, 223
+STYLE DS_SETFONT | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
 CAPTION "Dialog"
 FONT 14, "System", 0, 0, 0x0
 BEGIN
-    DEFPUSHBUTTON   "OK",IDOK,129,7,50,14
-    PUSHBUTTON      "Cancel",IDCANCEL,129,24,50,14
-    LISTBOX         IDC_LISTBOX,7,7,104,49,LBS_USETABSTOPS | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP
-    LTEXT           "Static",IDC_LISTTEXT,7,63,172,15
+    DEFPUSHBUTTON   "OK",IDOK,300,7,30,14
+    PUSHBUTTON      "Cancel",IDCANCEL,300,24,30,14
+    LISTBOX         IDC_LISTBOX,4,7,266,196,LBS_USETABSTOPS | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP | LBS_NOTIFY
+    LTEXT           "Static",IDC_LISTTEXT,4,207,172,15
 END


```
