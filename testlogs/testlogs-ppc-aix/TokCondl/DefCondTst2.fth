\  Modified Command-line-defined conditional test.
\      Using it to create anomalies for the detokenizer...
\  Updated Wed, 29 Jun 2005 at 13:43 by David L. Paktor
\
\  Symbol moogoo is either defined true (-1) or false (0), or is absent

[ifndef] moogoo
   f[ 
   .( Y'gotta define MooGoo on the command-line.)
   ." "!"
   ." X
a line."  ." Another on the line."
   ." "t(Lower-case will be okay, too)"
   f]
      #message Use  -D moogoo=true   or  -D moogoo=false
     [message]		or even  -D moogoo=-1   or  -D moogoo=0
     [#message]
[else]


tokenizer[ 

h#   feeb     \   Bogus Rev-Level
      SET-REV-LEVEL

not-last-image

h#   1fad     \   Vendor
h#   c0ed     \   Bogus Device ID
h#  90210     \   Bogus Class Code
     pci-header

         ]tokenizer

fcode-version2

   f[ 
       [defined] moogoo
   f]
fload TokConstCondTst02.fth

fcode-end

tokenizer[ 
   reset-symbols
	 ]tokenizer

fcode-version2

   f[ 
       [defined] moogoo  0=
   f]
fload TokConstCondTst02.fth

." De-tokenize THIS, wise-guy!"n"

tokenizer[ 
    0   emit-byte            \    Fake a premature end0
    6   emit-byte            \    Just to be confusing!
    h# 77   emit-byte
	 ]tokenizer

fcode-end

pci-header-end

tokenizer[ 

h#   2a55     \   Bogus Rev-Level
      SET-REV-LEVEL

     last-image

h#   5afe     \   Vendor
h#   1991     \   Bogus Device ID
h#  10203     \   Bogus Class Code
     pci-header

         ]tokenizer

fcode-version2

tokenizer[   reset-symbols  true     ]tokenizer

fload TokConstCondTst02.fth

tokenizer[ 
    \  Fake Fcode-Block header in the middle of things...
    h# f1  emit-byte		\   Fake start-byte
        8  emit-byte		\   Fake format
    h# 21  emit-byte		\   Fake checksum
    h# 95  emit-byte
        0  emit-byte		\   Fake length
        0  emit-byte
        0  emit-byte
    h# 12  emit-byte

   reset-symbols
   false
         ]tokenizer
   reset-symbols
fload TokConstCondTst02.fth

fcode-end

pci-header-end

[then]

