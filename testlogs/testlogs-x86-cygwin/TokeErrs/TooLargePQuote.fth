\  Test a P-Quote that is not terminated at all and whose number
\      of characters until the end of file exceeds the buffer.
\  A P-Quote has  the string-escape sequences.
\  Define the required  test-token  as alias to  ."
\      then FLOAD the test-body file.

\  Updated Tue, 09 May 2006 at 10:52 PDT by David L. Paktor

global-definitions
alias  test-token   ."

fcode-version2

headers


: foobar  
    fload  LargeTextNoQte.fth
;


fcode-end
