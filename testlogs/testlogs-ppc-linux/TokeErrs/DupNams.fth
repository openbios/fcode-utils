\  Let's make a batch of duplicate definitions in various scopes...

\  Updated Thu, 12 Oct 2006 at 13:17 PDT by David L. Paktor

\  Tracing:  alley-oop   boop   croop   doop   drop   dup   foop
\            gloop   _harpo   hoop   koop   loop   noop   poop
\            shtoop   zoop
\            fontbytes    blink-screen    base   bell

\  alley-oop
\  boop
\  croop	Global Macro
\  doop
\  drop		Built-in word, aliased, invoked
\  dup		Built-in word
\  foop 	Global alias to dup
\  floop	Global Macro, Alias to flop (alias to drop),
\                   redefined in subordinate device
\  gloop	Undefined, invoked
\  _harpo	Local, in subordinate device
\  hoop
\  koop
\  loop		Built-in word
\  noop 	Built-in word, redefined in second FCode block
\  poop
\  shtoop
\  zoop
\  fontbytes		Built-in VALUE
\  blink-screen 	Built-in DEFER
\  base 		Built-in VARIABLE
\  bell 		Built-in CONSTANT


alias foop dup

[macro] croop  foop #message" Using FOOP "

alias dup croop

\  Stubs.
\  These should be global.
\  But, then, we _are_ injecting errors for this test....

[macro] _{local}  noop  #message" I got yer ""Local"" right here, chum!"

alias {pop-locals} 3drop

f[  h# a5519e  constant  {push-locals}  ]f


global-definitions

alias flop drop
#message" Sync Up Diffs w/ prev. Release."n"n"
[macro] floop   flop #message" Using FLOOP "
alias drop floop

device-definitions

f[  false constant  o'ryan  ]f

fcode-version2

: noop  ."  Op?  No!" ;
[macro] zoop noop #message" I Care."
: poop  h# -21013572  ;

    new-device
       : zoop  ." Nothing like the other zoop"  croop ;
       : croop ." Sort of like F-Troop with a higher GPA..."
	  foop
	  drop
	  floop
       ;
       : foop  ." Shop bop-a-looma, a-lop bam boom!" ;
       : floop ." Oh, Jiggly!" ;
       : boop { _harpo | _cheeko }
	   f[  127 constant _harpo ]f
	   poop -> _cheeko
	   f[  _cheeko constant a__gent ]f
	   floop
       ;

       alias droop drop
       alias drupe 2drop

       boop    f[  true constant  o'ryan  ]f
       foop  03 constant 3 

       new-device
	   : moop
        	_harpo
		droop
		drupe
		boop
		floop
	   ;
	   alias shoop encode-int

	    f[
		7a63 constant octal
		boop
		char-height
		eval
		moop
		shoop
	     ]f

       finish-device

       : stoop 
            floop
	    gloop
	    shoop
       ;
       alias coop floop
       : troop
           shoop
	   coop
	   poop
       ;

    finish-device
start4  \  Let's just stick in an extra, and another error besides...
fcode-end

\ And a few false-starts and ends...
fcode-end

start0
   ." What does this button do?"
end1

start2
   ." Oh, was I not supposed to touch that?"
end0

save-image poop.fc   \  Not gonna happen anyway...
