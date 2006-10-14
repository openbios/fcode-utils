\  Test a Paren-Message that is not terminated at all and whose
\      number of characters until the end of file exceeds the buffer.
\  A Paren-Message does not have the string-escape sequences.
\  Enter Tokenizer-escape mode
\      then define the required  test-token  as alias to  .(
\      and FLOAD the test-body file.

\  Updated Tue, 09 May 2006 at 10:52 PDT by David L. Paktor

headers
global-definitions
   f[

alias  test-token  .(

    fload  LargeTextNoQte.fth

    ]f

