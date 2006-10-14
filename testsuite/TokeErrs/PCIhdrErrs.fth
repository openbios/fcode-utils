\  Basic PCI-Headers Errors test.

\  Updated Wed, 25 May 2005 at 17:30 by David L. Paktor


hex
tokenizer[

   c020
       SET-REV-LEVEL
   dec1    \  Vendor
   c0ed    \  Device ID
 80201    \  Class Code  (ISA system timer.  Denver Colorado  )

         ]tokenizer
." What is this?"

     pci-header

fcode-version2
headers
   : bogo  " This is a test."  ;


fcode-end

pci-header-end

