\  See if names from a previous tokenization linger in a subsequent one.

\  Updated Wed, 06 Apr 2005 at 11:32 by David L. Paktor

fcode-version2

headers
: peril ( new-val -- )
    dup to  twenty
    ['] peril  is do-nothing
    d# 17 is naught
;


fcode-end
