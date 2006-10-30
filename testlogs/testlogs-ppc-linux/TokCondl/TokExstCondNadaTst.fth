\  What happens to "Exists" Conditionals test when the
\       target isn't on the same line?
\  "Exists" side of test

\  Updated Tue, 21 Feb 2006 at 15:33 PST by David L. Paktor


fcode-version2

headers

create HotNasty

fload TokExstCondNada_01.fth

Global-definitions
    alias [yestest]  [ifexist]
device-definitions

fload TokExstCondNada_01.fth

Global-definitions
    alias [notest]  [ifnexist]
device-definitions

fload TokExstCondNada_01.fth

\  Here we will test:
\      Whether aliases to comment-delimiters are recognized:
\           in Normal mode
\           in Tokenizer-escape mode
\           inside conditional-compilation sections
\  We will also re-demonstrate the fate of Conditional-Operators
\      that occur inside conditional-compilation sections that are
\      being ignored.

Global-definitions
    alias //  \
device-definitions

\  #message" This is cleanly commented-out and will be ignored"
//  #message" This, too, will be ignored"
f[  \  #message" Comment safely ignored in Tok-esc mode"
    //  #message" Aliased comment in Tok-esc mode.  Should be ignored."
 ]f
[ifnexist] dup
    \   This section should be ignored consistently
    \  #message" This conditional commented-out message will be ignored"
    [message] The next message has a brack-then
    [message] [then] #message" Should be ignored but isn't"
    #message"  Re-balance the conditional..."  [ifnexist] dup
    //  Unprocessed Aliased comment. [then] #message" Faked-out"  [ifnexist] dup

[else]
    #message" This will not be ignored."
    //  #message" Aliased comment in unignored section"

[then]

fcode-end
