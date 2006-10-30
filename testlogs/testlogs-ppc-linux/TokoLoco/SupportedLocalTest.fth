\ SupportedLocalTest.fth
\    Updated Thu, 29 Sep 2005 at 11:34 PDT by David L. Paktor


fcode-version2

headers
hex 20 drop
decimal 32 drop
octal 40  drop
decimal
o# 40 40 2drop
d# 32 32 2drop
h# 20 20 2drop

d# 32 constant _local-storage-size_

fload LocalValuesSupport.fth


: faber ( n3 n2 n1 -- alloc-addr size $addr,len )
     { _otter _weasel _skunk ; _muskrat _mole }
   \  _otter is initialized with the value of n3
   \  _weasel is initialized with the value of n2
   \  _skunk is initialized with the value of n1
   \  _muskrat and _mole are uninitialized

   \  Use n1 to determine an amount of memory to allocate, and
   \  stuff the address into _mole
   _skunk 40 * -> _muskrat
   _muskrat alloc-mem  -> _mole
   _weasel .h type
   _otter  .d  type
   _mole _muskrat _mole count
;

: miracle ( n3 n2 n1 -- m3 m2 m1 )
   { _curly _larry _moe | _shemp _besser _joe }
     ." Nyuk! " _curly .h cr
     ." Why, you... " _moe .d cr
     ." Ouch! " _larry  . 
     _curly _moe + -> _shemp
     _larry _moe + -> _besser
     _besser _curly + -> _joe
     _joe _besser   8  faber type free-mem
     _moe _larry    8  faber type free-mem
     _curly _besser 8  faber type free-mem
     _larry _shemp
;

: ordinary ( -- )
   123 456 789 miracle
;

fload SupportedLocalTest2.fth

h# ordinary

fcode-end

