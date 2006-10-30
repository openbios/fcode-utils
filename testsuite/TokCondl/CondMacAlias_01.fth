\  Test Conditionals and Locals with Macros and Aliases
\      without (intentional) errors.  Common code...
\  Updated Fri, 10 Mar 2006 at 17:18 PST by David L. Paktor


(  File that floads this will have to define  // as an alias to \ and
    also (((   as an alias to (  (open-paren)

(((  That last also acts as the close-paren that finishes the two-line
    comment above.  Funny...  We also need  a macro called  [ifitis]
    whose meaning may change but which will always begin the "true"
    segment of a conditional, a macro called [udderwise] which will
    be the equivalent of  [else]  and one called [donewidit] which
    will be the equivalent of  [then]  )
//  Oh, yeah. And  loc{  as an alias to {  (open-curly-brace)

//  we will also have various comments, remarks and messages
\   with embedded false  [else]'s and the like...

[ifitis]
    #message"  Dis is da TROO side of da test.  What [else]  ya gonna do?
              Da message goes on ta heayuh!"
    f[  .(  Yuh got anuddah message.
             What [then] chum? )  ]f
    [ifitis]
        #message" Now yer in da TROO o' da TROO.  What [then] ?"
    [udderwise]
        #message" Yer in da FLASE o' da TROO.  Yer in trubble, chum!"
    [donewidit]
    #message" Back to da foist level o' da TROO.  Let's try sump'in'..."
    : contralto  ((( tony vinnie looie -- marie )
	   #message" No warning for multiline decl'n and none for comment"
	   multi-line
       loc{  _ay_tony
	   multi-line    (((  He's da ringleadah
                              Y'know, da leader of da pack!  )
             _ay_vinnie  //  He's da mussel
	     _ay_bobby   (  He's got da ringwoim )
	     |  _ay_marie  (((  She's my goil!  )
       }
        _ay_tony  _ay_bobby +  _ay_vinnie *
	->  _ay_marie 
	#message" Multiline warning for this comment"
	(  Dey're all outa roo'beah!
	   Y'wanna Doctah Peppah instea'?  )  \  Got used by dis instea'?
	 _ay_marie
     ;
     : alto ((( sis boom bah -- hahaha )
        #message" Warning for multiline decl'n but not comment"
	   loc{   _sis _boom
	          _bah  multi-line  (((  What is this anyway?
		       Oh, right...   )   |  _hahaha
	      }
	  #message" Warning for this multiline comment"  (((  What is this?
	       It is a whiz.  )
          _sis _bah * _boom - dup -> _hahaha
     ;
     : tenor ( jose placido luciano -- enrico josef )
        #message" No warning for multiline decl'n, but one for comment"
	multi-line
       loc{  _jose      ((( Can you see?
                            Buy the Donzerly light!  )
	        _placido _luciano
		 | _enrico _josef
         }
	 _placido _luciano + _jose / -> _josef
     ;
		  
[udderwise]
  #message" Dis is da FLASE side of da test.  What ya gonna do [then] ? 
              Ya gonna let da message go on ta heayuh!"
    f[  .( So dis is anuddah message.
             What [else] chum? )  ]f
    [ifitis]
        #message" Dis is da TROO o' da FLASE.  Y'shouldn'a'ought'a evuh be heayuh..."
	f[  ."  Let's try some spurious [else] action, whaddya say?"  ]f
	: [else]  ."  Don't do it" ;  [message]  Fake [else] got through
	create [else]   [message]  Fake [else] got through
	h# DeFeCA8e constant [else]   [message]  Fake [else] got through
	h# -41100132 value [else]   [message]  Fake [else] got through
	d# 64  buffer: [else]   [message]  Fake [else] got through
	 struct
	 /l field [else] [message]  Fake [else] got through
	variable [else]  [message]  Fake [else] got through
	defer [else]    [message]  Fake [else] got through
	 ['] [else]     [message]  Fake [else] got through
	   to [else]    [message]  Fake [else] got through
	 ['] [else]   alias moomoo [else]
	   is [else]    [message]  Fake [else] got through
	   : say-what ( tony vinnie looie -- 
	                   -- marie )
\		multi-line  \ Should refer to the loc's decl'n, not to the comment
       loc{  _ay_tony
	   multi-line  \  Y'want this to refer to the comment
	        //  but when ignoring, what happens?
                         (((  He's da ringleadah
                              Y'know, da leader of da pack!  )
             _ay_vinnie  //  He's da mussel
	     [else]         [message]  Really bad fake [else] got through
	     _ay_bobby   (  He's got da ringwoim )
	     |  _ay_marie  (((  She's my goil!  )
       }
        _ay_tony  _ay_bobby +  _ay_vinnie *
	->  _ay_marie 
	(  Dey're all outa roo'beah!
	   Y'wanna Doctah Peppah instea'?  )  \  Got used by dis instea'?
	 _ay_marie
     ;
     .( What does an [else] do in dot-parens? )  [message]  Fake [else] got through
     ." What does an [else] do in dot-quotes?"   [message]  Fake [else] got through
     s" What does an [else] do in Ess-quote?"    [message]  Fake [else] got through
      " What does an [else] do in dbl-quotes?"   [message]  Fake [else] got through
     
    [udderwise]
        #message" Dis is da FLASE o' da FLASE.  Yer [then] should be absawbed bot' ways."
    [donewidit]
   [message]  Y'should be back to the FLASE side.  Okay, [then]
[donewidit]


