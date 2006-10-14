\  Create an image with multiple PCI headers
\     and then let's see what still we need to do
\
\   Updated Mon, 23 May 2005 at 16:17 by David L. Paktor


tokenizer[ 

h#   f2a7     \   Bogus Rev-Level
      SET-REV-LEVEL

not-last-image

h#   1fed     \   Vendor
h#   9009     \   Bogus Device ID
h#  20109     \   Bogus Class Code
     pci-header

	 ]tokenizer

fload TokConstCondTstT.fth

pci-header-end

     

tokenizer[ 

h#   ea57     \   Bogus Rev-Level
      SET-REV-LEVEL

     last-image

h#   deaf     \   Vendor
h#   9021     \   Bogus Device ID
h#  10902     \   Bogus Class Code
     pci-header

	 ]tokenizer

fload TokConstCondTstF.fth

pci-header-end

