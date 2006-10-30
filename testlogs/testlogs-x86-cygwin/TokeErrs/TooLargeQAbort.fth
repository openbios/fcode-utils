\  Test an Abort-Quote whose body is not terminated at all and whose
\      number of characters until the end of file exceeds the buffer.
\  Abort-Quote may be dis-allowed.  This test allows it.
\  Define the required  test-token  as alias to  abort"
\      then FLOAD the test-body file.

\  Updated Wed, 10 May 2006 at 11:41 PDT by David L. Paktor

\  Align with counterpart...

global-definitions
alias  test-token   abort"

fcode-version2

headers

: barfalot
    true
    fload  LargeTextNoQte.fth
;

\  Let's also see how a disallowed abort" is handled when the string is
\      legit but crosses several lines
: ohfooey!
    true
 #message" The abort"" starts here."       abort"  This ... "\
     is another fine mess "\
         you've gotten me into.  "\
	     Or is it a kettle of fish?"
;


fcode-end
