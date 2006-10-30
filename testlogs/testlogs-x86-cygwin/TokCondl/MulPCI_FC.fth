\  Another multiple PCI and FCode test

\  Updated Fri, 10 Jun 2005 at 09:06 by David L. Paktor


[ifndef] first-path \ Check to see if the symbol was defined?
  F[
 ." "n"n"tAdd a command-line switch:"n"t"t-d ""first-path=<true|false>"" 
 "tthen run this again."n"n"
    F]
[else]
  F[
    b00b  c0ed  90210   pci-header
       [defined] first-path 
     F]
[message] Loading first pass
    not-last-image

    fload TokConstCondTst01.fth

    pci-header-end


F[   reset-symbols
    b00b  fece  07112   pci-header
       [defined] first-path  0= 
     F]
[message] Loading second pass
    last-image

    fload TokConstCondTst01.fth

    pci-header-end

[then]

