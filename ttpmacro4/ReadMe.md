# ttpmacro : VT window のテキスト領域の位置やサイズを取得する getttpos コマンド追加版

  [パッチ](https://github.com/TeraTermProject/teraterm/pull/269/files)  
  [バイナリ](https://github.com/hkanou/ttpmacro/tree/main/ttpmacro4/Release)

# 使用方法

```
getttpos <x> <y> <width> <height> <minimized>

  Tera TermのVT windowの下記の値を取得します。(単位:ピクセル)

  <x>         テキスト領域左上隅の X 座標
  <y>         テキスト領域左上隅の Y 座標
  <width>     テキスト領域の幅
  <height>    テキスト領域の高さ
  <minimized> VT windowの最小化状態(0:通常状態、1:最小化状態)

  ※ VT windowが最小化されている場合は、<x> <y> <width> <height>には誤った値が設定されます。
```

getttposで取得したx,yをsetdlgposのx,yで指定するとテキスト領域左上隅にダイアログが表示されます。


```
  testlink
  if result > 1 then
  :start
    getttpos x y width height minimized
    if minimized = 1 then
      showtt 1
      goto start
    endif
    setdlgpos x y
    sprintf2 str 'x=%d, y=%d, width=%d, height=%d' x y width height
    messagebox str 'x,y = top-left of the text area'
  endif
```
