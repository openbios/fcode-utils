\  Test a Quote-Message that is not terminated at all and whose
\      number of characters until the end of file exceeds the buffer.
\  A Quote-Message has the string-escape sequences.
\  Define the required  test-token  as alias to  #message"
\      then FLOAD the test-body file.

\  Updated Tue, 09 May 2006 at 10:52 PDT by David L. Paktor

global-definitions
alias  test-token  #message"


headers

    fload  LargeTextNoQte.fth

