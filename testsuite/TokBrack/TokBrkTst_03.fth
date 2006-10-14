\  Test new Aliasing algorithm

\  Updated Tue, 31 May 2005 at 12:05 by David L. Paktor

fcode-version2
tokenizer[ 
\  emit-date
\  alias dte emit-date
alias dq  ."
dq  This is a message"
\  dte
h# 00030000  constant goodmeat
goodmeat fliteral
alias goodeats goodmeat
goodeats  fliteral
reset-symbols
	 ]tokenizer
end0
