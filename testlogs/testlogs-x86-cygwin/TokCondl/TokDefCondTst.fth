\  Stand-alone version of Nested "Defined" Conditionals test
\      looking for command-line or [define] def'n of  NestTest

\  Updated Wed, 27 Apr 2005 at 17:59 by David L. Paktor

fcode-version2

." Begin Nest Test Test"
[ifdef] NestTest
   ." Is defined, level 1"
   [ifdef] NestTest
	." Is defined and is defined, level 2"
   [else]
	." Is defined but isn't defined.  What?  Level 2"
   [then]
   ." Resumes defined-ness, level 1"
   [ifndef] NestTest
	." Is defined but is not defined.  What?  Level 2"
   [else]
	." Is defined and isn't not defined, level 2"
   [then]
   ." Still is defined, level 1"
[else]
    ." Isn't defined, level 1"
    [ifdef] NestTest
	." Isn't defined but is defined.  What?  Level 2"
    [else]
	." Isn't defined and isn't defined, level 2"
    [then]
    " Resumes non-defined-ness, level 1"
    [ifndef] NestTest
	." Isn't defined and is not defined, level 2"
    [else]
	." Isn't defined but isn't not defined.  What?  Level 2"
    [then]
   ." Still isn't defined, level 1"
[then]

." Middle of Nest Test Test"
[ifndef] NestTest
    ." Is not defined, pass 2, level 1"
    [ifdef] NestTest
	." Is not defined but is defined.  What?  Pass 2, Level 2"
    [else]
	." Is not defined and isn't defined, pass 2, level 2"
    [then]
    " Resumes non-defined-ness, pass 2, level 1"
    [ifndef] NestTest
	." Is not defined and is not defined, pass 2, level 2"
    [else]
	." Is not defined but isn't not defined.  What?  Pass 2, Level 2"
    [then]
   ." Still is not defined, pass 2, level 1"
[else]
    ." Isn't not defined, pass 2, level 1"
    [ifdef] NestTest
	." Isn't not defined and is defined, pass 2, level 2"
    [else]
	." Isn't not defined but isn't defined.  What?  Pass 2, Level 2"
    [then]
    " Resumes not non-existing, pass 2, level 1"
    [ifndef] NestTest
	." Isn't not defined but is not defined.  What?  Pass 2, Level 2"
    [else]
	." Isn't not defined and isn't not defined, pass 2, level 1"
    [then]
   ." Still isn't not defined, pass 2, level 1"
[then]

." End Nest Test Test"


fcode-end
