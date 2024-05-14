# ttpmacro

  - **listbox修正版**  
    [ttpmacro_mod.exe](https://github.com/hkanou/ttpmacro/tree/main/ttpmacro/Release)、
    [使用方法](http://htmlpreview.github.io/?https://github.com/hkanou/ttpmacro/blob/main/ttpmacro/doc/listbox.html)、
    [パッチ](https://github.com/hkanou/ttpmacro/tree/main/ttpmacro)。
    (動作環境 : [Tera Term 5](https://teratermproject.github.io/))

    ttpmacro(teraterm-5.2)のlistboxに下記の修正を行ったものです。
    - アイコン画像の設定
    - ダブルクリックによる項目選択オプションの追加
    - 最小化/最大化ボタンオプション追加
    - 最小化状態表示オプション追加
    - 最大化状態表示オプション追加
    - ダイアログサイズ指定オプションの追加

    ![Listbox Image](image/listbox.png)

  - **setpassword、getpassword暗号化機能追加版**  
    [ttpmacro_enc.exe](https://github.com/hkanou/ttpmacro/tree/main/ttpmacro2/Release)、
    [使用方法](http://htmlpreview.github.io/?https://github.com/hkanou/ttpmacro/blob/main/ttpmacro2/doc/setpassword.html)、
    [パッチ](https://github.com/hkanou/ttpmacro/tree/main/ttpmacro2)。
    (動作環境 : [Tera Term 5](https://teratermproject.github.io/))

    ttpmacro(teraterm-5.2)のsetpassword、getpasswordのパスワードをAES-256-CTRで暗号化/復号するためのパッチです。

  - **ファイル操作コマンド暗号化機能追加版**  
    [パッチ](https://github.com/hkanou/ttpmacro/tree/main/ttpmacro3)。
    (動作環境 : [Tera Term 5](https://teratermproject.github.io/))
  
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

## ビルド環境

  Visual Studio Express 2017
