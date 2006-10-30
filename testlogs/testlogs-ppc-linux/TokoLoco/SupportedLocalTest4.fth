\ SupportedLocalTest4.fth
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


: faber ( m4 m3 n2 n1 n0 -- alloc-addr size $addr,len )
        {    _otter          \  _otter is initialized with the value of n2
             _weasel         \  _weasel is initialized with the value of n1
             _skunk          \  _skunk is initialized with the value of n0
                             \  It will be used to determine
                             \      an amount of memory to allocate
                 ( Vertical Bar ends the group of Initialized Locals  )       |     ( m3 and m4 stay on the stack )
                       \  These are uninitialized:
             _muskrat      \  final size of the allocation
             _mole         \  address of the allocated memory
                            }

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

h# ordinary

fcode-end

