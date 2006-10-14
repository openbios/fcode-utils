\  Multiple FCode images under one PCI header
\  False, then True

\  Updated Wed, 01 Jun 2005 at 14:51 by David L. Paktor

tokenizer[ 

h#   fa57     \   Bogus Rev-Level
      SET-REV-LEVEL

h#   cede     \   Vendor
h#   193a     \   Bogus Device ID
h#  95014     \   Bogus Class Code

	 ]tokenizer
     pci-header


fload TokConstCondTstF.fth

tokenizer[ 
   reset-symbols
	 ]tokenizer

fload TokConstCondTstT.fth


pci-header-end
