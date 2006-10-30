\ testcase3.fth
\    Updated Wed  2 Mar 2005 at 09:54 by David L. Paktor

marker fuggedaboudit

: foobar ( n3 n2 n1 -- m1 )
     { _eenie _meany _miney ; _moe }
   ." Eenie = " _eenie . cr
   ." Meany = " _meany . cr
   ." Miney = " _miney . cr
   _meany _miney + _eenie * -> _moe
   " throw" confirmed? cr throw
   ." Moe = " _moe . cr
   _moe
;

: goobar ( n3 n2 n1 -- m1 )
     { _eenie _meany _miney ; _moe }
   ." GEenie = " _eenie . cr
   ." GMeany = " _meany . cr
   ." GMiney = " _miney . cr
   _meany 2* _eenie 2* _miney 2* foobar 10 + -> _moe
   ." GEenie = " _eenie . cr
   ." GMeany = " _meany . cr
   ." GMiney = " _miney . cr
   ." GMoe = " _moe . cr
   _moe
;

: loobar ( n3 n2 n1 -- m1 )
     { _eenie _meany _miney ; _moe }
   ." LEenie = " _eenie . cr
   ." LMeany = " _meany . cr
   ." LMiney = " _miney . cr
   _miney 2* _meany 2* _eenie 2* goobar 10 + -> _moe
   ." LEenie = " _eenie . cr
   ." LMeany = " _meany . cr
   ." LMiney = " _miney . cr
   ." LMoe = " _moe . cr
   _moe
;

: hoobar ( n3 n2 n1 -- m1 )
     { _eenie _meany _miney ; _moe }
   ." HEenie = " _eenie . cr
   ." HMeany = " _meany . cr
   ." HMiney = " _miney . cr
   _eenie 2* _miney 2* _meany 2* loobar 10 + -> _moe
   ." HEenie = " _eenie . cr
   ." HMeany = " _meany . cr
   ." HMiney = " _miney . cr
   ." HMoe = " _moe . cr
   _moe
;

: poobar ( n3 n2 n1 -- m1 )
     { _eenie _meany _miney ; _moe }
   ." PEenie = " _eenie . cr
   ." PMeany = " _meany . cr
   ." PMiney = " _miney . cr
    _miney 2* _meany 2* _eenie 2*
   ['] hoobar catch if 
       ." Caught"  3drop false
   else  10 + -> _moe  true
   then  cr
   ." PEenie = " _eenie . cr
   ." PMeany = " _meany . cr
   ." PMiney = " _miney . cr
   if
   ." PMoe = " _moe . cr  _moe
   else  0
   then  cr

;


: ordinary ( -- )
   123 456 789 poobar
   .s cr
;
 
