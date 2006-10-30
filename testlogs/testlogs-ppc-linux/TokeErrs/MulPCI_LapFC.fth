\  Combine Multiple PCI headers with overlapping FCodes.
\  Updated Fri, 01 Sep 2006 at 12:55 PDT by David L. Paktor

\  We already have a source with overlapping FCodes in two FCode blocks
\  We just have to wrap it in multiple PCI headers.
\  Also, we have to make sure to define  NoCrash  as a command-line symbol
\  While we're at it, let's also create another switchable behavior:
\  If the command-line symbol  RangeTwo  is defined, it should have 
\      a value in the form of a hex number (We'll take care of the "hex")
\      which will, in a roundabout way, become the starting FCode for
\      the second loading of the common Source.
\  We'll save the symbol in a roundabout manner that will test whether
\      a particular means can be used to preserve the current FCode
\      assignment counter across PCI blocks, especially when there is
\      an  fcode-reset  associated with the end of a PCI block.
\  Oh, and...  We'll control whether the  fcode-reset  is called by another
\      command-line symbol:  If NoReset  is defined, we will bypass
\      issuing the  fcode-reset  command.  (We expect we'll be making it
\      automatic and implicit at the end of a PCI block, but we're still
\      testing the premise...)


tokenizer[ 

h#   f2a7     \   Bogus Rev-Level
      SET-REV-LEVEL

not-last-image

h#   1fed     \   Vendor
h#   9009     \   Bogus Device ID
h#  20109     \   Bogus Class Code
     pci-header

	 ]tokenizer

fload TooManyFCodes.fth


\  Generate an error or two if  RangeTwo  is not DEFINED.  It's harmless
     f[
	[DEFINED]  RangeTwo  constant SecondRangeFCode
      ]f
\  because all the other references to  SecondRangeFCode  are conditioned...

\  Try it once before and once after...    
[ifndef]  NoReset  fcode-reset [else] #message" Not resetting..." [endif]
pci-header-end  [ifndef]  NoReset  fcode-reset  [endif]

tokenizer[ 

h#   ea57     \   Bogus Rev-Level
      SET-REV-LEVEL

     last-image

h#   deaf     \   Vendor
h#   9021     \   Bogus Device ID
h#  10902     \   Bogus Class Code
     pci-header

	 ]tokenizer


[ifdef] RangeTwo
     f[
	SecondRangeFCode next-fcode
      ]f
[endif]

fload TooManyFCodes.fth


pci-header-end  [ifndef]  NoReset  fcode-reset  [endif]
