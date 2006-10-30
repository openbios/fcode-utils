\  Third Local-overrun test.  Unterminated locals decl'n w/ no separator

: coconuts ( m1 m2 -- m3 m4 m5 )
    { 
         q 
	    _gummo
	        _karl

    2dup < if swap then
    2dup / -> _gummo
    2dup - -> _karl
    * -> q
    _karl _gummo q
;
