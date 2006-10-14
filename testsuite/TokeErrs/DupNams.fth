\  Let's make a batch of duplicate definitions in various scopes...

\  Updated Fri, 02 Jun 2006 at 09:48 PDT by David L. Paktor

alias foop dup

[macro] croop  foop message" Using FOOP "

alias dup croop

\  Stubs.
\  These should be global.
\  But, then, we _are_ injecting errors for this test....

[macro] _{local}  noop  #message" I got yer ""Local"" right here, chum!"

alias {pop-locals} 3drop

f[  h# a5519e  constant  {push-locals}  ]f


global-definitions

alias flop drop
[macro] floop   flop message" Using FLOOP "
alias drop floop

device-definitions

f[  false constant  o'ryan  ]f

fcode-version2

: noop  ."  Op?  No!" ;
[macro] zoop noop message" I Care."
: poop  h# -21013572  ;

    new-device
       : zoop  ." Nothing like the other zoop"  croop ;
       : croop ." Sort of like F-Troop with a higher GPA..."
	  foop
       ;
       : foop  ." Shop bop-a-looma, a-lop bam boom!" ;
       : floop ." Oh, Jiggly!" ;
       : boop { _harpo | _cheeko }
	   f[  127 constant _harpo ]f
	   poop -> _cheeko
	   f[  _cheeko constant a__gent ]f
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
