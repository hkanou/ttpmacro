# VT window �̃e�L�X�g�̈�̈ʒu��T�C�Y���擾���� getttpos �R�}���h�ǉ���

  [�p�b�`](https://github.com/TeraTermProject/teraterm/pull/269/files)

# �g�p���@

```
getttpos <x> <y> <width> <height> <minimized>

  Tera Term��VT window�̉��L�̒l���擾���܂��B(�P��:�s�N�Z��)

  <x>         �e�L�X�g�̈捶����� X ���W
  <y>         �e�L�X�g�̈捶����� Y ���W
  <width>     �e�L�X�g�̈�̕�
  <height>    �e�L�X�g�̈�̍���
  <minimized> VT window�̍ŏ������(0:�ʏ��ԁA1:�ŏ������)

  �� VT window���ŏ�������Ă���ꍇ�́A<x> <y> <width> <height>�ɂ͌�����l���ݒ肳��܂��B
```

getttpos�Ŏ擾����x,y��setdlgpos��x,y�Ŏw�肷��ƃe�L�X�g�̈捶����Ƀ_�C�A���O���\������܂��B


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
