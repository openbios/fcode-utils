\  Test an imbalanced control statement before a device-node change


fcode-version2
hex
headers

alias rc! rb!

[message]  Top-Level (root) device-node
create achin  12 c, 13 c, 14 c,
: breakin  achin 3 bounds do i c@ . loop ;
: creakin     0 if breakin then ;
: deacon   achin creakin drop breakin ;

[message]  Control structure starts here
3 0 do  
    i ." loop number" .
[message]  Forgot the "loop" here.

[message]  Subsidiary (child) device-node
new-device
create eek!  18 c, 17 c, 80 c, 79 c,
: freek  eek! 4 bounds ?do i c@ . 1 +loop ;
: greek
        recursive -1 if ." By name" greek
                        ." other name" freek
        else  ." Re-Curse you!"  recurse
	then
;
[message]  About to access method from parent node
: hierareek
       eek!
           freek
	       achin
	           greek
;
: ikey  hierareek  freek  greek ;
[message]  about to end child node
[message] But first a bogus incomplete control structure
false if
    ." This should not be happening"
[message] "Forgot" the then here...
finish-device
[message]  We can access methods from the root node now
: jeeky
     achin
       3 type
;
[message] Proved our point I think.

[message] Another bogo-crol
    begin   ." What the hey?"  0 while
        ."  Forgot it again..."
[message]  Missing repeat...


fcode-end
