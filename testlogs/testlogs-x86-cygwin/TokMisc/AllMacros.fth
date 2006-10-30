\  Quick test for all single-function macros that have just been moved...
\  This code isn't executable in any sense; but a detokenization
\    should show that each "original" function is displayed twice
\  (With the exception of the last pair, which takes a little more...)

fcode-version2

" Start of simple pairs"
    not
    invert
    <<
    lshift
    >>
    rshift
    na1+
    cell+
    /c*
    chars
    /n*
    cells
    flip
    wbflip
    version
    fcode-revision
    true
    -1
    false
    0
    struct
    0
    eval
    evaluate
    u*x
    um*
    xu/mod
    um/mod
    x+
    d+
    x-
    d-
    attribute
    property
    xdrint
    encode-int
    xdr+
    encode+
    xdrphys
    encode-phys
    xdrstring
    encode-string
    xdrbytes
    encode-bytes
    decode-2int
    parse-2int
    map-sbus
    map-low
    name
    device-name
    get-my-attribute
    get-my-property
    xdrtoint
    decode-int
    xdrtostring
    decode-string
    get-inherited-attribute
    get-inherited-property
    delete-attribute
    delete-property
    get-package-attribute
    get-package-property
    wflips
    wbflips
    lflips
    lwflips
" End of simple pairs."
0 if " Last phrase-item"
    endif
0 if " Last phrase-item"
    then

fcode-end
