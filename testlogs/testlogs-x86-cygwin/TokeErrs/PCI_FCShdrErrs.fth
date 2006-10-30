\  Combination PCI-Headers Errors test.

\  Updated Thu, 08 Sep 2005 at 17:31 by David L. Paktor


hex
tokenizer[

   c020
       SET-REV-LEVEL
   dec1    \  Vendor
   c0ed    \  Device ID
 80201    \  Class Code  (ISA system timer.  Denver Colorado  )

         ]tokenizer

     pci-header

." What is this?"   \  Output FCode before the  fcode-version2 .  S.b. error...

fcode-version2
headers
   : bogo  " This is a test."  ;


fcode-end

pci-header-end

