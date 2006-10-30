\ Test whether the Local-Values Support file will tokeniz ok
\      with a Global setting -- and an error.

\  Updated Mon, 23 Jan 2006 at 18:50 PST by David L. Paktor

fcode-version2

external
h# 30 constant  _local-storage-size_
\   Ooopsie!
\   That's not the same scope as where LocalValuesSupport.fth will look for it!
headers
instance variable gumbage
instance variable guggley
instance variable burglar
instance variable hot-dog
: update  " Updated Mon, 23 Jan 2006 at 18:50 PST by David L. Paktor" ;

global-definitions
fload LocalValuesSupport.fth
fload LocalValuesDevelSupport.fth
device-definitions

: downdate ['] update catch ;

fcode-end

