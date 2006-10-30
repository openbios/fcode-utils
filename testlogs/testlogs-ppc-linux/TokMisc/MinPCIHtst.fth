\  Minimal basic PCI-Header test.

\  Updated Tue, 24 May 2005 at 11:22 by David L. Paktor

hex
tokenizer[

   c020
       SET-REV-LEVEL
   dec1    \  Vendor
   c0ed    \  Device ID
 a8d2e1    \  Class Code
     pci-header
         ]tokenizer


fcode-version2

headers
   : bogo  " This is a test."  ;


fcode-end

pci-header-end
