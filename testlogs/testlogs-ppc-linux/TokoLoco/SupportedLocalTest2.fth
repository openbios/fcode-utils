\ SupportedLocalTest2.fth
\     Updated Fri, 18 Mar 2005 at 14:47 by David L. Paktor

: ducksoup ( n1 n2 n3 n4 -- m1 m2 )
	{ _harpo _chico    \
	  _groucho _zeppo }
    _groucho _harpo *
    _chico _zeppo +
    _groucho _zeppo = if  swap exit then
    _groucho + swap _zeppo + 
;

: coconuts ( m1 m2 -- m3 m4 m5 )
          {     (  No initted locals ) ; q \  Try a one-character name
	                 _gummo   \  The little-known Marx brother
			 _karl    \  Was he part of the comedy act, too?
			  }
    2dup < if swap then
    2dup / -> _gummo
    2dup - -> _karl
    * -> q
    _karl _gummo q
;

: spaulding ( x y -- u v w ){ _lfn _pjs | _ill _never _know }
      _pjs _lfn - ->  _never
      _never  _pjs * -> _ill
      _ill _lfn / -> _know
      _ill _never _know
 ;

: dumont ( a b -- c )
     {  _dont _ask
         |  _why  }
      _ask _dont /  ->  _why
       _why _ask - _dont *
;
