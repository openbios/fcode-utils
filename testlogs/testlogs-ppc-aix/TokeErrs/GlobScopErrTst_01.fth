\  Test the Global-Scope directive and a few of its effects.
\       Updated Thu, 12 Jan 2006 at 15:36 PST by David L. Paktor
\  GlobScopErrTst_01.fth  --  very slight variant on  GlobScopErrTst.fth 

fcode-version2

headers

[ifexists] coconuts
   [message]  Why a duck?
[endif]

alias [testdict]  [ifexists]

[testdict] coconuts
   [message]  Boy, can you get stucco!
[else]
   [message]  Why a fence?
[endif]

\  Bypass Instance warning:  Not right; only has scope in top-level dev-node
alias generic-instance  instance
overload [macro] instance  f[  noop  .( Shminstance!) f]  noop
\  This will only work if user-macros always have "global" scope
\     I think they should follow the common rules for scope; this should fail

global-definitions

fload LocalValuesSupport.fth

\  Replace normal meaning of  Instance
overload alias instance generic-instance

: $CAT   ( _max _str1 _len1 _str2 _len2 -- _max _str1 _len1' )
   { _max _str1 _len1 _str2 _len2 }
   _len1 _max < if                  \ there is room
      _str2 _str1 _len1 + _len2 _max _len1 - min move
   then
   _max _str1 _len1 _len2 + _max min \ always leave total length
;

instance variable fussel [message] Expected error; scope is still global.
h# 3afe fussel !

device-definitions

h# 5afe instance value dumont [message] Top Device-node scope.  S.b. Legit.
: ducksoup ( n1 n2 n3 n4 -- m1 m2 )
	{ 	\  Declare some locals
	   _harpo  ( the quiet one) _chico
	    _groucho |   \  He's funny, right?
	     _zeppo  ( who? ) _karl  \  Is he part of the act?
	      }
    d# 64 _groucho dup count dup -> _zeppo
    _harpo dup count $cat
    dup -> _karl
    rot _karl = if  type exit then
    _groucho + swap _zeppo + 
;

global-definitions

: garbanzo
      ." Should be unrecognized." cr
      ducksoup
;

new-device    [message] Missing a finish-device

: fazooule!
     ."  Lima enter tain you..." cr
     ducksoup
     garbanzo
;

h# deadc0de  instance value quaack   [message] Should be worng "Instance" here.

global-definitions

: frijoles
    ." Holy ... beans?" cr
    fazooule!
    garbanzo
    ducksoup
;

finish-device

finish-device

fcode-end
