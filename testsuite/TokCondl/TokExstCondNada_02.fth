\  What happens to "Exists" Conditionals test when the
\       target isn't on the same line?
\  Or the "Defined" Conditionals, for that matter
\  Common code (insert obligatory sneeze here)
\  FLoaded only after necessary preliminary test.

\  Updated Tue, 21 Feb 2006 at 15:58 PST by David L. Paktor

." Begin Nest Test Test"
[yestest]
   ." Exists, level 1"
   [yestest]
	." Exists and exists, level 2"
   [else]
	." Exists but doesn't exist.  What?  Level 2"
   [then]
   ." Resumes existence, level 1"
   [notest]
	." Exists but not exists.  What?  Level 2"
   [else]
	." Exists and doesn't not exist, level 2"
   [then]
   ." Still exists, level 1"
[else]
    ." Doesn't exist, level 1"
    [yestest]
	." Doesn't exist but exists.  What?  Level 2"
    [else]
	." Doesn't exist and doesn't exist, level 2"
    [then]
    " Resumes non-existence, level 1"
    [notest]
	." Doesn't exist and not exists, level 2"
    [else]
	." Doesn't exist but doesn't not exist.  What?  Level 2"
    [then]
   ." Still doesn't exist, level 1"
[then]

." Middle of Nest Test Test"
[notest]
    ." Not exists, pass 2, level 1"
    [yestest]
	." Not exists but exists.  What?  Pass 2, Level 2"
    [else]
	." Not exists and doesn't exist, pass 2, level 2"
    [then]
    " Resumes non-existence, pass 2, level 1"
    [notest]
	." Not exists and not exists, pass 2, level 2"
    [else]
	." Not exists but doesn't not exist.  What?  Pass 2, Level 2"
    [then]
   ." Still not exists, pass 2, level 1"
[else]
    ." Doesn't not exist, pass 2, level 1"
    [yestest]
	." Doesn't not exist and exists, pass 2, level 2"
    [else]
	." Doesn't not exist but doesn't exist.  What?  Pass 2, Level 2"
    [then]
    " Resumes not non-existing, pass 2, level 1"
    [notest]
	." Doesn't not exist but not exists.  What?  Pass 2, Level 2"
    [else]
	." Doesn't not exist and doesn't not exist, pass 2, level 1"
    [then]
   ." Still doesn't not exist, pass 2, level 1"
[then]

." End Nest Test Test"
