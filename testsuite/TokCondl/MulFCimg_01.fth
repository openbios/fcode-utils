\  Another test of conditionals and multiple FCode blocks.
\  This is the "Wrapper" file.

\  Updated Wed, 10 Aug 2005 at 10:57 by David L. Paktor


[ifndef] moogoo
   f[ 
   .( Y'gotta define MooGoo on the command-line.)
      #message Use  -D moogoo=true   or  -D moogoo=false
   f]
[else]

    tokenizer[ 

    h#   5afe     \   Bogus Rev-Level
	  SET-REV-LEVEL

    h#   beef     \   Vendor
    h#   c0de     \   Bogus Device ID
    h#  90210     \   Beverly Hills ZIP Code.  Now that's a _Class_ Code!

	 pci-header
	     ]tokenizer

       f[ 
	   [defined] moogoo
	   constant  boobalah?
       f]

    fcode-version2

    fload MulFCimg_01_Body.fth

    fcode-end


       f[ 
       reset-symbols
	   [defined] moogoo
	   0=
	   constant  boobalah?
       f]

    fcode-version2

    fload MulFCimg_01_Body.fth

    fcode-end

    pci-header-end

[then]
