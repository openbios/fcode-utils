\  Test a Parenthesis-Comment that is not terminated at all and whose
\      number of characters until the end of file exceeds the buffer.
\  Define the required  test-token  as alias to  (
\      then FLOAD the test-body file.

\  Updated Fri, 01 Sep 2006 at 09:39 PDT by David L. Paktor


global-definitions
alias  test-token   (

start4

headers


: foobar  
    fload  LargeTextNoQte.fth
;


fcode-end
