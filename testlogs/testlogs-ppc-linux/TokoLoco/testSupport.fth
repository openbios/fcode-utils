\ Test whether the Local-Values Support file will tokenize ok.

fcode-version2

external
h# 30 constant  _local-storage-size_
headers
instance variable gumbage
instance variable guggley
instance variable burglar
instance variable hot-dog
: update  " Updated Thu, 29 Sep 2005 at 11:34 PDT by David L. Paktor" ;

fload LocalValuesSupport.fth

: downdate ['] update catch ;

fcode-end

