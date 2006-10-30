\  Pointer-overrun:  unterminated Locals Declaration  

\  Updated Fri, 08 Jul 2005 at 11:55 by David L. Paktor

fcode-version2

headers

fload LocalValuesSupport.fth


fload  LocOvrRun_01.fth

\  Supply the lost semicolon:
;

fload  LocOvrRun_02.fth

\  Supply another lost semicolon:
;

fload  LocOvrRun_03.fth

[message]  Back to main file

\  Supply yet another lost semicolon:
;

fload  LocOvrRun_04.fth

fcode-end

