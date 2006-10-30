\  Source file that controls duplicate loading of itself
\  Updated Thu, 27 Jul 2006 at 15:24 PDT by David L. Paktor


f[  [ifnexist]  DupLoadBody.fth
    true constant DupLoadBody.fth   f]

."  Just this one time, eh!" cr

f[   .( Go ask your mother)  ]f


f[  [endif]   f]
