\  Test multiple device-nodes with injected erorrs... ;-}
\  MulDev_02.fth  --  slight variant relative to  MulDev_01.fth 

\  Updated Thu, 12 Jan 2006 at 15:36 PST by David L. Paktor

global-definitions
   alias rc! rb!
device-definitions

fcode-version2

headers

[message]  Top-Level (root) device-node
create achin  12 c, 13 c, 14 c,
: breakin  achin 3 bounds do i c@ . loop ;
: creakin     0 if breakin then ;
: deacon   achin creakin drop breakin ;

[message]  Subsidiary (child) device-node
new-device
create eek!  18 c, 17 c, 80 c, 79 c,
: freek  eek! 4 bounds ?do i c@ . 1 +loop ;
: greek  -1 if  freek then ;
[message]  About to access method from parent node
: hierareek
       eek!
           freek
	       achin
	           greek
;
: ikey  hierareek  freek  greek ;
[message]  about to end child node
finish-device
[message]  We can access methods from the root node now
: jeeky
     achin
       3 type
;
[message] create sibling node
new-device
0 value inky
: kinky  
    " "(   \
    \  value  offset   
    03  22   \   Comm Params (offset 22) = parity check (bit 0) even (bit 1) 
    4   17   \   Plex  (offs 17) = full (bit 2)
    b7  0e   \   Bells (offs 0e) = Bits 7,6,4,2,1,0  (No church or Gamelon)
    7f  0f   \  Bell volume (offs 0f)  = Just under halfway
    89  10   \  Whistles (offs 10) = Foghorn, Train, Piccolo (Bits 7,3,0)
    ff  18   \  Foghorn duration (offs 18) = maximum
    22  14   \  Train-whistle (offs 14) = two short blasts
    03  11   \  Piccolo = mercifully short
          )"
    bounds do i c@ i 1+ c@  inky + rc! 2 +loop
;

[message] creating nephew node
new-device

: open  kinky true ;

[message] creating great-nephew node
new-device

: open
     jeeky
	 kinky
             true ;

fcode-end
