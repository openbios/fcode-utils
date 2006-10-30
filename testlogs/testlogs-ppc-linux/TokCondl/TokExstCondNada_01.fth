\  What happens to "Exists" Conditionals test when the
\       target isn't on the same line?
\  Or the "Defined" Conditionals, for that matter
\  Preliminary test before FLoading Common code (insert obligatory sneeze here)

\  Updated Tue, 21 Feb 2006 at 15:33 PST by David L. Paktor


\  File that FLoads this must define a pair of macros or aliases
\      called  [yestest]  and  [notest]  respectively, that are
\      inverses of each other.

f[  false  ]f
[ifnexist] [yestest]
    [if]    [endif]    \   Until we have a better way to  drop  in tok-esc mode
    f[   true  ]f
[endif]

[ifnexist] [notest]
    [if]    [endif]    \   Until we have a better way to  drop  in tok-esc mode
    f[   true  ]f
[endif]
[#message] got this far
[if]
    [#message] \ Must define macros or aliases called  [yestest]  and  [notest]  respectively
[else]
[#message] are we here
    fload TokExstCondNada_02.fth
[#message] we are here
[endif]

