\  Test aliasing of Conditional directives.  Use Command-line-definition
\  Updated Tue, 20 Dec 2005 at 16:15 PST by David L. Paktor

\
\  Symbol moogoo is either defined true (-1) or false (0), or is absent


alias [dowegotit?]  [ifdef]
alias [ifyouaintgot] [ifndef]
alias [izzatso?]   [if]
alias [udderwise]  [else]
alias [donewidit]  [then]
   f[   alias  mess(  .(
        alias  mess"  ."
   f]

[ifyouaintgot] moogoo
   f[ 
   mess( Y'gotta define MooGoo on da command-line.)
   mess" "n"tIt's eider  -D moogoo=true   or  -D moogoo=false"
   f]
[udderwise]
   f[ 
      mess( Hey!  Y'got MooGoo!  Good for you!)
      [defined] moogoo  [izzatso?]
	 mess" And guess what!  It's TRUE!  Drinks all around!"
      [udderwise]
	 mess( So wut's it gonna be?  You gonna be FLASE to me?)
      [donewidit]
   f]
[donewidit]

[dowegotit?]  moogoo
   f[ 
   mess( Hey!  Did I menshun dat we got MooGoo?)
   mess" "n"tYeah?  Well, so wut if I did?  I'm gonna menshun it again!
	WE GOT MooGoo!!!"
   f]
[udderwise]
   f[ 
   mess" "n"tCuz if ya don' got dat MooGoo, it don't mean a t'ing..."   f]
[donewidit]
