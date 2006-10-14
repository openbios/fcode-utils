\  Test of various formats of strings

\  Updated Tue, 12 Jul 2005 at 17:19 by David L. Paktor

fcode-version2

headers hex

."
Empty string next."
." "
." BSlashes: \t\1fea9\abdc\n\1f\\fece"
decimal
." BSlashes: \t\a7\c01a"
hex
." BSlashes: \n\a7\c01a"
." BSlashes: \t\a7\\c0\\1a"
." BSlashes: \t\a7\\c0\\1a"( feedface)"
." 3 BSlashes, then QOpen. \t\Q\n"(090abcdefeca8e beeffece b020)Zoh. "(1 23 4 567 8 9 0 1 2 3 0 a b c 30)"
.( Dot-Paren-NoSpace)cr cr
.( Dot-Paren Space) cr cr
." QEscapes: "p"b"n"zz"
." QEscapes: ""Q"nn"rr"tt"ff"ll"bb"!!"^[UpBrack"zz"
0 is my-self                 \  Is it keeping line numbers straight?
" Quote"" Quote" type cr
s" Ess-Quote"type cr
." Cross
the
line.
Three times."
0 is my-self                 \  Is it still keeping line numbers straight?
." Can I get a \ backslash?"
." Like this \\ maybe?"
." What about "( 1c cd e6 \ The rest of this line should be a comment
         c7  )?"
."  Is "\ the rest of this line a comment?"
\  "  \  This should do it.
0 is my-self                 \  Is it still keeping line numbers straight?
." Or a way to get a "\ backslash?"
\  "  \  Or not...
0 is my-self                 \  Is it still keeping line numbers straight?

fcode-end
