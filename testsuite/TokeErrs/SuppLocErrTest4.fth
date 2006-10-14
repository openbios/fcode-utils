\ SuppLocErrTest4.fth
\    Updated Fri, 30 Jun2006 at 14:09 PDT by David L. Paktor

fcode-version2

headers

alias snatch catch

d# 32 constant _local-storage-size_

fload LocalValuesSupport.fth

overload : catch  snatch catch ;
variable dup

: ducksoup ( n1 n2 n3 n4 -- m1 m2 )
	{ 	\  Declare some locals
	   _harpo  ( the quiet one) _chico
	    _groucho |   \  He's funny, right?
	     _zeppo  ( who? ) _karl  \  Is he part of the act?
	      }
    _groucho _harpo *
    _chico +
    _groucho _zeppo = if  swap exit then
    _groucho + swap _zeppo + 
;

: cluckpoop ( n1 n2 n3 n4 flag? -- m1 m2 )
    if   ." Freedonia's going to war!" cr then
	{ 	\  Declare some locals after the def'n has started
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

: neighcluck ( n1 n2 n3 n4 flag? -- m1 m2 )
    	{ 	\  Declare some locals
	   _harpo  ( the quiet one) _chico
	    _groucho |   \  He's funny, right?
	     _zeppo    ( who? ) _karl  \  Is he part of the act?
	    _zeppo   \  Not again...
	    ;   \  What is that?
	    _gummo
	     _karl  (  Another repeater )   }

    ." I'm against it!" cr
    _groucho _harpo *
    _chico +
    _groucho _zeppo = if  swap exit then
    _groucho + swap _zeppo + 
     -> _gummo
    ." What's yours is mine..."  cr
    dup -> _karl
;
: coconuts ( m1 m2 -- m3 m4 m5 )
    { ; q _gummo _karl }
    instance                \   No,
    create                  \  No, no no!
    2dup < if swap then
    2dup / -> _gummo
    2dup - -> _karl
    * -> q
    _karl _gummo q
;
	
_harpo
_chico
_groucho
_zeppo
_gummo
_karl

\  Two sets of Locals Declarations in one definition
: spaulding
    ."  Hooray for the captain!" cr
    { _lfn _pjs _how | _got _in  }
    
    _how  _lfn + -> _got
    123 -> _in  _pjs
    { _I'll _never _know }
    _in  _pjs _I'll _never _know
;

\  Locals Declarations in a structure
\  Inside an outer structure...

true if
    : wire-fence
	."  Inside a structure?"
	if
            { _why _a _duck | _viaduct }
	     _duck _why +  _a * -> _viaduct
	     _viaduct 0= if exit then
	     ." Why a duck?"
	then
	."  Because water..."
	\  Scope of Locals should end here but it doesn't.
	_a _duck _why * + _viaduct = if  ." Any takers?" exit then
	." Step right up"
    ;
then

fcode-end
