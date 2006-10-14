\  Test a Parenthesis-Comment that is not terminated at all and the number
\      of characters until the end of file exceeds the buffer.
\  Define the required  test-token  as alias to  (
\      then FLOAD the test-body file.

\  Updated Wed, 10 May 2006 at 10:43 PDT by David L. Paktor


global-definitions
alias  test-token   (

fcode-version2

headers


: foobar  
    fload  LargeTextNoQte.fth
;


fcode-end
