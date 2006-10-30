\  String-Escapes test...

\  Updated Mon, 11 Jul 2005 at 16:44 by David L. Paktor


fcode-version2

headers hex

." What about "( 1c cd e6 \ The rest of this line should be a comment
         c7  )?"
."  Is "\ the rest of this line a comment?"
X and a new start "  \  This should do it.
0 is my-self                 \  Is it still keeping line numbers straight?
." Or a way to get a "\ backslash?"
 Y not  "  \  Or not...
0 is my-self                 \  Is it still keeping line numbers straight?

fcode-end
