\ LocalTest1.fth
\    Updated Thu, 12 Jan 2006 at 13:35 PST by David L. Paktor

fcode-version2
	\  Deliberately omitting inclusion of  LocalValuesSupport
	\  Correct inclusion is in  TokoLoco/SupportedLocalTest.fth
: ducksoup ( n1 n2 n3 -- m1 m2 )
	{ _harpo _chico _groucho | _zeppo _karl }
    _groucho _harpo *  -> _zeppo  dup
    _chico _zeppo +  -> _karl     dup
    _groucho _zeppo = if  swap exit then
    _groucho + swap _zeppo + 
;

: coconuts ( m1 m2 -- m3 m4 m5 )
    { ; q _gummo _karl }
    2dup < if swap then
    2dup / -> _gummo
    2dup - -> _karl
    * -> q
    _karl _gummo q
;

fcode-end
