# ttpmacro

  - **listbox修正版**  
    [パッチ](https://github.com/TeraTermProject/teraterm/commit/7b3ccb4fe2999557aa7e17d6e0ceeb15156b9633)
    (動作環境 : [Tera Term 5](https://teratermproject.github.io/))

    ttpmacro(teraterm-5.2)のlistboxに下記の修正を行ったものです。  
    → Tera Termのソースにマージ頂きました。https://github.com/TeraTermProject/teraterm/pull/222

    - アイコン画像の設定
    - ダブルクリックによる項目選択オプションの追加
    - 最小化/最大化ボタンオプション追加
    - 最小化状態表示オプション追加
    - 最大化状態表示オプション追加
    - ダイアログサイズ指定オプションの追加
  
  - **setpassword、getpassword暗号化機能追加版**  
    [パッチ](https://github.com/TeraTermProject/teraterm/commit/e7d5453bfb6813567b24d90692b79e4c0060949a)
    (動作環境 : [Tera Term 5](https://teratermproject.github.io/))

    ttpmacro(teraterm-5.2)のsetpassword、getpasswordのパスワードをAES-256-CTRで暗号化/復号するためのパッチです。  
    → Tera Termのソースにマージ頂きました。https://github.com/TeraTermProject/teraterm/pull/211

  - **ファイル操作コマンド暗号化機能追加版**  
    [パッチ](https://github.com/hkanou/ttpmacro/tree/main/ttpmacro3)。
    (動作環境 : [Tera Term 5](https://teratermproject.github.io/))
  
    **<ins>※ 試作中</ins>**  
  
    暗号化されたファイルの読み書き等を可能とするパッチです。  
    
    filecreate \<file handle\> \<filename\> **<ins>['password=abcd']</ins>**  
    fileopen \<file handle\> \<filename\> \<append flag\> [\<readonly flag\>] **<ins>['password=abcd']</ins>**  
    fileconcat \<file1\> \<file2\> **<ins>['password1=abcd'] ['password2=abcd']</ins>**  
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

# ttpmenu

  - **ログインパスワードの暗号化機能追加版**  
    [ttpmenu_mod.exe](https://github.com/hkanou/ttpmacro/tree/main/ttpmenu1/Release)  
    ![ttpmenu Image1](ttpmenu1/image/ttpmenu1.png)  
    ![ttpmenu Image2](ttpmenu1/image/ttpmenu2.png)

## ビルド環境

  Visual Studio Express 2017
