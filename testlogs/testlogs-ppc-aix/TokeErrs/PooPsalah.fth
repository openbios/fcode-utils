\  FLoadable body for various conditional tests, based on the value of
\      a constant named "poopsalah?"  (Okay, so I was running out of ideas...)

." Begin Nested ""Constant"" Test Test"
f[ poopsalah? f] [if]
[message]  The TRUE path has wisely been chosen, Poopsalah!
   ." Is True, level 1"
   f[ poopsalah? f] [if]
       " Is True and is True, level 2"
   [else]
       " Is True but is Not True.  What?"
   [then]
   ." Middle of level 2"
   f[ poopsalah? 0= f] [if]
       " Is True and is False.  What?"
   [else]
       " Is True and is Not False, level 2"
   [then]
[else]
[message]  Oy, Poopsalah.  You have foolishly chosen the FALSE path.
   ." Is Not True, level 1"
   f[ poopsalah? f] [if]
       " Is Not True but is True.  What?"
   [else]
       " Is Not True and is Not True, level 2"
   [then]
   ." Middle of Not level 2"
   f[ poopsalah? 0= f] [if]
       " Is Not True and is False, level 2"
   [else]
       " Is Not True and is Not False.  What?"
   [then]
\  Missing a [then]

