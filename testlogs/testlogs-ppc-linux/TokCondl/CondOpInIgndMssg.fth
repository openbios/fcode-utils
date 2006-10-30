\  Here we will demonstrate the fate of Conditional-Operators
\      that occur in the text-body of user-messages that occur
\      in conditional-compilation sections that are being ignored.
\  We will also test whether aliases to comment-delimiters are recognized:
\      in Normal mode
\      in Tokenizer-escape mode
\      inside conditional-compilation sections

\  Updated Wed, 22 Feb 2006 at 12:52 PST by David L. Paktor

fcode-version2

headers

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

garblegarblegarble   //  Force an error.

fcode-end

