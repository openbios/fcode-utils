\  Test overlapping FCode # error checking, and reaching the max allowable.

\  Updated Wed, 06 Sep 2006 at 18:23 PDT by David L. Paktor

\  A macro to force showing the current  nextfcode
global-definitions
    [macro]  show-next-fcode  fcode-push  [if]  [then] #message" ... and dropped off."
    [ifnexist] fcode-reset
      [macro] fcode-reset  #message" Faking FCODE-RESET" f[ h# 800 next-fcode ]f
    [endif]
device-definitions

fcode-version2

decimal
headers
    show-next-fcode

0 constant  my_zero
fload OneTwoFive.fth
  fcode-push f[ f['] eleven   next-fcode  ]f fcode-pop  fcode-push
#message" one_twenty-six"    126 constant  one_hundred_and_twenty-six
#message" one_twenty-seven"  127 constant  one_hundred_and_twenty-seven
#message" one_twenty-eight"  128 constant  one_hundred_and_twenty-eight
             f[ f['] eleven   emit-fcode  ]f
    show-next-fcode

    new-device
	fcode-reset
	0 constant  my_zero
	fload OneTwoFive.fth
	#message" one_twenty-six"    126 constant  one_hundred_and_twenty-six
	#message" one_twenty-seven"  127 constant  one_hundred_and_twenty-seven
	#message" one_twenty-eight"  128 constant  one_hundred_and_twenty-eight

	f[ fcode-push f['] eleven   next-fcode ]f  fcode-pop
    finish-device

    new-device
	f[  h# 08c0  next-fcode  ]f
	0 constant  my_zero
	fload OneTwoFive.fth
	f[  fcode-push constant dev-1-lap   ]f
	#message" one_twenty-six"    126 constant  one_hundred_and_twenty-six
	#message" one_twenty-seven"  127 constant  one_hundred_and_twenty-seven
	#message" one_twenty-eight"  128 constant  one_hundred_and_twenty-eight
	show-next-fcode
    finish-device

    new-device
	fcode-pop
	0 constant  my_zero
	fload OneTwoFive.fth f[ fcode-push f['] eleven next-fcode fcode-pop ]f 
	f[  fcode-push constant dev-2-lap   ]f
	#message" one_twenty-six"    126 constant  one_hundred_and_twenty-six
	#message" one_twenty-seven"  127 constant  one_hundred_and_twenty-seven
	#message" one_twenty-eight"  128 constant  one_hundred_and_twenty-eight
	show-next-fcode
    finish-device

fcode-push   \  Can we do this across FCode Blocks?
\  And, if we can't, can we preserve it this way?
f[  constant XFcBlkFcd
    XFcBlkFcd    \  Push it back...
 ]f

    new-device
	f[  dev-1-lap  next-fcode  ]f
	0 constant  my_zero
	fload OneTwoFive.fth
	fcode-push
	#message" one_twenty-six"    126 constant  one_hundred_and_twenty-six
	fcode-pop
	#message" one_twenty-seven"  127 constant  one_hundred_and_twenty-seven
    finish-device
    show-next-fcode

fcode-end

\  Have to redefine these...
global-definitions
    [macro]  show-next-fcode  fcode-push  [if]  [then] #message" ... and dropped off."
    [ifnexist] fcode-reset
      [macro] fcode-reset  #message" Faking FCODE-RESET" f[ h# 800 next-fcode ]f
    [endif]
device-definitions

fcode-version2
	\   Confirm that the FCode numbers continue across FCode Blocks
	show-next-fcode
	#message" one_twenty-eight"  128 constant  one_hundred_and_twenty-eight

\  Let's confirm that the reset clears out the lapping messages.
    new-device
	\  This is not a reset:
	f[   h# 800 next-fcode  ]f
	0 constant  my_zero

	\  This is:
	fcode-reset
	fload OneTwoFive.fth

    finish-device

\  Can we use what we pushed on the other side of the block?
f[  constant wanna-pop
    wanna-pop   fcode-pop  wanna-pop   0= 
    \  Did it succeed?  If not, there's no point...
 ]f [if]
       #message" Could not use FCode pushed on other side of block"
       #message" Try this...  "   f[  XFcBlkFcd  fcode-pop  ]f
       
    [else]  \  It sucked seed!
	new-device
	0 constant  my_zero
	fload OneTwoFive.fth
		show-next-fcode
	finish-device
    [endif]

\  Final run:  Exceed the FCode # limit and crash.
\  Rather than load all the numbers from the start,
\      let's get a jump on the FCode # assignments.
f[   h# f80 next-fcode  ]f

\  Get all but the last few...

0 constant  my_zero
fload OneTwoFive.fth

	show-next-fcode
#message" one_twenty-six"   126 constant  one_hundred_and_twenty-six
	show-next-fcode
#message" one_twenty-seven" 127 constant  one_hundred_and_twenty-seven
	show-next-fcode

\  This next one pushes the FCode # over the limit and causes a crash.
\  Let's leave ourselves a way around that, so we can use this in other ways
\  Allow a command-line symbol called NoCrash to prevent this.
[ifndef] NoCrash
    #message" one_twenty-eight"  128 constant  one_hundred_and_twenty-eight
	show-next-fcode
[else] \  Otherwise, let's do this test:
    #message" Overflow the data-stack."
    f[  decimal   fload  TooManyPushes.fth  f]
[endif]

fcode-end
