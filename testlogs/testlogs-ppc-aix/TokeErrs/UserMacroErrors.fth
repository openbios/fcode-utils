\  Test Error-catching for user-defined macros and other new features.
\  Including: erroneous attempt at recursive macro invocation...
\  Updated Tue, 17 Jan 2006 at 11:25 PST by David L. Paktor

global-definitions

[macro] lookma
[macro]
[macro] lookpa  .( Hey, Pa!  Hands! HaHa! ) 
[macro] heylookmeover  .( What's clover?)  \  It's money, honey!
[macro] lookout .( Look out, look out look out! 
[macro] f[looknoquote  f[ ." I forgot
#message  Are you ready?
alias foop dup
overload [macro] dup  #message" Faking a DUP here"  foop

device-definitions

fcode-version2
headers

lookpa
heylookmeover
lookpa

lookout
f[looknoquote ]f

: whatzit
   heylookmeover
   dup
   to heylookmeover
   dup
   to 2+
;

   a#

#message  Here comes a little bit of macro recursion.
[macro] foop  #message" It's a call to dup, but which one?"  dup

: now-what?
    #message  Don't try this at home, kids...
    dup
;

fcode-end
