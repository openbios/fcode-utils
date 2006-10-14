\  A few simple string-escape sequences.
\     See which ones the "Old" tokenizer handles differently...


fcode-version2

." Let's also test a few string-escape sequences:"
." "ttab"nnew-line""quote"rreturn"fform-feed"bbackspace"!bell"
." "^Dcontrol-D"^{Control-brace is Escape"

."  First try these two in hex:"
hex

." Backslash-n\nNewLine\tTab\1234And-a-OneTwoThreeFour"
." Backslashes on both ends:\1234\OneTwoThreeFour"

."  Try them again, but now in decimal:"
decimal
." Backslash-n\nNewLine\tTab\1234And-a-OneTwoThreeFour"
." Backslashes on both ends:\1234\OneTwoThreeFour"

fcode-end
