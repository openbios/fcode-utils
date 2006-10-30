\  Test case of a C-String-Escape clause that overflows the source-file

[flag] C-Style-string-escape

fcode-version2

headers
hex

: funky-string
   ." Normal string" cr
   ." Funky but ok \ab\\b0\\7\\7\and\c0\\5\\7\\e1\\10\" cr
   ." Body 1" cr
fload CStrEscOvflw_body_1.fth
   ." Body 2" cr
fload CStrEscOvflw_body_2.fth
   ." Body 3" cr
fload CStrEscOvflw_body_3.fth

    ." I don't know."  cr  ." Shortstop!" cr
;

fcode-end
