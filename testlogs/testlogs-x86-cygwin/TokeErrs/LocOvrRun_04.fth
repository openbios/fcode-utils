\  Fourth Local-overrun test.  Unterminated Mis-placed locals decl'n


: cluckpoop ( n1 n2 n3 n4 flag? -- m1 m2 )
    if   ." Freedonia's going to war!" cr then
	{ 	\  Declare some locals
	   _harpo  ( the quiet one) _chico
	    _groucho |   \  He's funny, right?
	     _zeppo    ( who? ) _karl  \  Is he part of the act?
	    _zeppo   \  Not again...
	    ;   \  What is that?
	   }
    _groucho _harpo *
    _chico +
    _groucho _zeppo = if  swap exit then
    _groucho + swap _zeppo + 
;

: horse-feathers ( n1 n2 n3 n4 flag? -- m1 m2 )
    if   ." I'm against it!" cr then

	{ 	\  Declare some locals
	   _harpo  ( the quiet one) _chico
	    _groucho |   \  He's funny, right?
	     _zeppo    ( who? )
	     _karl  \  Is he part of the act?
	     \  No terminating close-curly-brace
    _groucho _harpo *
    _chico +
    _groucho _zeppo = if  swap exit then
    _groucho + swap _zeppo + 
;
