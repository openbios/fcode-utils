\  Erorrrs involving "instance"

\  Updated Thu, 19 Jan 2006 at 15:14 PST by David L. Paktor


fcode-version2

headers

instance

: goombah  dup swap drop ;
: foosbat  over swap nip ;
: casball  dup dup rot rot drop drop ;
    variable chump

instance

instance

new-device
  : what-the-heck  ."  What now?"  ;
  : how-now?  ."  Now what?"  ;
  0 value sclump

instance

finish-device

global-definitions
    237 buffer:  mugwump

device-definitions

global-definitions
    880 buffer:  nimnump
    " madmirable_" count
     dup nimnump c!
     dup constant nimbasesiz
      0 do dup i + c@ nimnump 1+ i + c! loop
      drop
      [macro] (u.h)  base @ hex swap (u.) rot base !
      [macro] nimnumprop numnim nimnump count encode-string " nimnum" property
      : numnim
           nimnump nimbasesiz +
	   my-address (u.h)
	   dup >r
	   0 do
	       2dup i + c@ swap i + c!
	   loop  2drop
	   r> nimbasesiz + nimnump c!
      ;

   : gummink
       dup
	 instance
            numnim
   ;

  instance

device-definitions

: prumpick
       dup
	 instance
            gummink
;

new-device

instance

    nimnumprop  

true instance value hardware-store

: knacknick
    nimnumprop
        instance
;

instance  knacknick

finish-device

instance

fcode-end
