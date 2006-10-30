\  Secondary test of tokenizer-escape mode functions 
\      Updated Thu, 02 Mar 2006 at 17:47 PST by David L. Paktor

[macro]  where-from   ." This came from " [input-file-name] type ."  line " [line-number] .d cr
[macro] in_what    ." In " [function-name] type cr
[macro] mess_in_what   f[  [function-name] ]f

tokenizer[ 

fload revlev.fth

h#   020000 constant eithernet    \    Class Code: 0x020000  (Ethernet)
h#     5417 constant deviouce     \    Device ID: 0x5417
h#     17d5 constant vanitor      \    Vendor ID: 0x17d5

 vanitor deviouce eithernet
 i-got-a-Rev-Level
 false
	 ]tokenizer

not-last-image
last-img
set-last-image
	   SET-REV-LEVEL
	     pci-header

fcode-version2

headers
hex
tokenizer[ 

d# 10 constant triumph
 o# 40 constant trophy
  h# 80 constant trumpet
	 ]tokenizer
b(lit)
tokenizer[ 
       10  emit-byte 
        triumph  emit-byte 
	   trumpet  emit-byte 
	       trophy  emit-byte 

	 ]tokenizer
b(lit)
tokenizer[ 

    h# de h# fe h# ca h# 8e
        2swap swap
	     emit-byte emit-byte
        swap emit-byte emit-byte

	 ]tokenizer

h# defeca8e  constant poopoo
h# beeffece  constant moopoo
alias couterde moopoo
tokenizer[ 
h# defeca8e  constant poopoo
h# beeffece  constant moopoo
alias couterde moopoo
    couterde  poopoo
	 ]tokenizer fliteral
     fliteral

    f['] moopoo  \  Can't  f['] couterde   just yet
    couterde  poopoo
tokenizer[ h# feedbac4   ]tokenizer   \  Leave something extra on the stack...
: merde
    ['] moopoo  \  Can't  ['] couterde   just yet, either 
     ' poopoo
    couterde
    ascii b  char e  [char] f
[message]  About to tokenize run-time date and time stamps.  Twice!
    ." Date " [fcode-date] type
    ." Time"  [fcode-time] type cr
    ." Time"  [fcode-time] type
    ." Date " [fcode-date] type cr
f[  [fcode-date]  [fcode-time]   ]f
   in_what  where-from  mess_in_what
;
ascii f
   char e
     [char] c

: terde
   in_what  where-from  mess_in_what
;


end0

\ save-image chalupa.fc

