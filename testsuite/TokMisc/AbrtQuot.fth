\  Test of Abort" with various switches, JTMS.
\  Throw in a few other misc goodies while we're at it...


fcode-version2

headers

h# defeca8e  constant poopoo
h# beeffece  constant moopoo
alias couterde moopoo

: gotta_try_it
    gumfritsch  \  Let's see how an unknown word is treated, before.
    {   \  What does this do?
     }   \  Or let's see what this does...
    couterde poopoo = abort" Should be different. "
    ." poopoo is "  f['] poopoo .h cr
    ." couterde is "  f['] couterde .h cr
    ." And its XT is: "  ['] couterde
    strumburkle  \  Let's see how an unknown word is treated after.
;

fcode-end
