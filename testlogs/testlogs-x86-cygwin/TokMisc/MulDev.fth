\  Test creating multiple device-nodes
\  Let's contrive this to (almost...) pass the "Original" tokenizer as well,
\      and display the contrast.
\  Updated Mon, 30 May 2005 at 19:44 by David L. Paktor

alias // \
//  Funny kind of comment.  What?  C-Plus style?  Not even a "B"...

fcode-version2

headers

create (sis 6 c, 8 c, 12 c, 
: err-shoot) ( -- 0 )  h# defeca8e  .h ;
: eatit(  h# feedface .h cr ;
: open ( -- success )
  err-shoot) 
   ."  No dice, Cholly." cr
   eatit(
   false
;

" sis" encode-string " name" property

finish-device
new-device

: eatit(  h# 900df00d .h cr ;            \  This should be a new definition

: open ( -- success )
    err-shoot)				\  This should be an "Unknown Word"
   ."  I'm sorry, Dave.  I can't do that." cr
   eatit(				\  This should be the above
   					\      new definition in any case.
   false
;

" boombah" encode-string " name" property

fcode-end
