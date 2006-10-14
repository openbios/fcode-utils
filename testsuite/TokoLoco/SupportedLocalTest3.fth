\ SupportedLocalTest3.fth
\    Updated Thu, 29 Sep 2005 at 11:34 PDT by David L. Paktor


fcode-version2

headers

d# 32 constant _local-storage-size_

fload LocalValuesSupport.fth

: ducksoup ( n1 n2 n3 n4 -- m1 m2 )
	{ _harpo _chico _groucho | _zeppo _karl }
    _groucho _harpo *
    _chico +
    _groucho _zeppo = if  swap exit then
    _groucho + swap _zeppo + 
;

fcode-end
