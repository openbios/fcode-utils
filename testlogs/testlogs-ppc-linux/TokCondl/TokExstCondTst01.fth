\  Common code (insert obligatory sneeze here)
\      for Nested "Exists" Conditionals test

\  Updated Tue, 12 Apr 2005 at 14:45 by David L. Paktor


." Begin Nest Test Test"
[ifexist] NestTest
   ." Exists, level 1"
   [ifexist] NestTest
	." Exists and exists, level 2"
   [else]
	." Exists but doesn't exist.  What?  Level 2"
   [then]
   ." Resumes existence, level 1"
   [ifnexist] NestTest
	." Exists but not exists.  What?  Level 2"
   [else]
	." Exists and doesn't not exist, level 2"
   [then]
   ." Still exists, level 1"
[else]
    ." Doesn't exist, level 1"
    [ifexist] NestTest
	." Doesn't exist but exists.  What?  Level 2"
    [else]
	." Doesn't exist and doesn't exist, level 2"
    [then]
    " Resumes non-existence, level 1"
    [ifnexist] NestTest
	." Doesn't exist and not exists, level 2"
    [else]
	." Doesn't exist but doesn't not exist.  What?  Level 2"
    [then]
   ." Still doesn't exist, level 1"
[then]

." Middle of Nest Test Test"
[ifnexist] NestTest
    ." Not exists, pass 2, level 1"
    [ifexist] NestTest
	." Not exists but exists.  What?  Pass 2, Level 2"
    [else]
	." Not exists and doesn't exist, pass 2, level 2"
    [then]
    " Resumes non-existence, pass 2, level 1"
    [ifnexist] NestTest
	." Not exists and not exists, pass 2, level 2"
    [else]
	." Not exists but doesn't not exist.  What?  Pass 2, Level 2"
    [then]
   ." Still not exists, pass 2, level 1"
[else]
    ." Doesn't not exist, pass 2, level 1"
    [ifexist] NestTest
	." Doesn't not exist and exists, pass 2, level 2"
    [else]
	." Doesn't not exist but doesn't exist.  What?  Pass 2, Level 2"
    [then]
    " Resumes not non-existing, pass 2, level 1"
    [ifnexist] NestTest
	." Doesn't not exist but not exists.  What?  Pass 2, Level 2"
    [else]
	." Doesn't not exist and doesn't not exist, pass 2, level 1"
    [then]
   ." Still doesn't not exist, pass 2, level 1"
[then]

." End Nest Test Test"

