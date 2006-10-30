\  Second Local-overrun test


: miracle ( n3 n2 n1 -- m3 m2 m1 )
   { _curly   \  Curly braces is why we think of these guys
     _larry
      _moe
       ;               \   Initted/un-initted separator present
       _shemp
        _besser
	 _joe
	 \  How many Three Stooges were there in all?
	 \     six!   }
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
