\  Abort-quote with embedded quote in the string.

\  Updated Tue, 16 May 2006 at 17:57 PDT by David L. Paktor



fcode-version2
hex
headers

: sunny-de-light
    abort" Oh, this is the Sun-Style abort"" with a "\
            built-in quote and multi-line!  Cool..."
;


[flag] noSun-ABORT-Quote

: apple-pi-alamo
    if  abort" And this is the Apple-Style abort"" with "\
            also a multi-line and built-in quote.  Ho, hum."
    then
;


fcode-end
