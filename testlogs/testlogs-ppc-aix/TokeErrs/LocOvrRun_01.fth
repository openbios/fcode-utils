\  First Local-overrun test


: ducksoup ( n1 n2 n3 n4 -- m1 m2 )
	{ 	\  Declare some locals
	   _harpo  ( the quiet one) _chico
	    _groucho |   \  He's funny, right?
	     _zeppo  ( who? ) _karl  \  Is he part of the act?
	      \  Look Ma, no close-curly brace!
	      (  Unterminated comment
	      }  \  Even if there were a close-curly brace,
	         \      the unterminated comment masks it.
    _groucho _harpo *
    _chico +
    _groucho _zeppo = if  swap exit then
    _groucho + swap _zeppo + 
;

