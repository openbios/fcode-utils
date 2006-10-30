\  Multiple FCode images under one PCI header
\  True, then False

\  Updated Wed, 01 Jun 2005 at 14:51 by David L. Paktor

tokenizer[ 

h#   fa57     \   Bogus Rev-Level
      SET-REV-LEVEL

h#   cede     \   Vendor
h#   193a     \   Bogus Device ID
h#  95014     \   Bogus Class Code

	 ]tokenizer
     pci-header


fload TokConstCondTstT.fth


fload TokConstCondTstF.fth


pci-header-end
