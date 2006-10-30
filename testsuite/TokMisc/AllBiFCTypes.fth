\  Test all the known Built-In FCode tokens
\     that have specific definition Types

\  Updated Tue, 10 Oct 2006 at 11:00 PDT by David L. Paktor

\  Applying "TO" to them ought to generate errors
\  except for the ones that legitimately take "TO",
\  namely the DEFER and VALUE definitions

fcode-version2

[message]  Constants.  Should generate errors
d# 10 to -1  (  CONST  )
d# 10 to -1  (  CONST  )
d# 10 to 0  (  CONST  )
d# 10 to 0  (  CONST  )
d# 10 to 0  (  CONST  )
d# 10 to 1  (  CONST  )
d# 10 to 2  (  CONST  )
d# 10 to 3  (  CONST  )
d# 10 to bell  (  CONST  )
d# 10 to bl  (  CONST  )
d# 10 to bs  (  CONST  )

[message]  Defer Words.  Should generate no errors
['] noop to  blink-screen  (  DEFER  )
['] noop to  delete-characters  (  DEFER  )
['] noop to  delete-lines  (  DEFER  )
['] noop to  draw-character  (  DEFER  )
['] noop to  draw-logo  (  DEFER  )
['] noop to  erase-screen  (  DEFER  )
['] noop to  insert-characters  (  DEFER  )
['] noop to  insert-lines  (  DEFER  )
['] noop to  invert-screen  (  DEFER  )
['] noop to  reset-screen  (  DEFER  )
['] noop to  toggle-cursor  (  DEFER  )

[message]  Value Words.  Should generate no errors
h# 32 to  #columns  (  VALUE  )
h# 32 to  #lines  (  VALUE  )
h# 32 to  char-height  (  VALUE  )
h# 32 to  char-width  (  VALUE  )
h# 32 to  column#  (  VALUE  )
h# 32 to  fontbytes  (  VALUE  )
h# 32 to  frame-buffer-adr  (  VALUE  )
h# 32 to  inverse-screen?  (  VALUE  )
h# 32 to  inverse?  (  VALUE  )
h# 32 to  line#  (  VALUE  )
h# 32 to  my-self  (  VALUE  )
h# 32 to  screen-height  (  VALUE  )
h# 32 to  screen-width  (  VALUE  )
h# 32 to  window-left  (  VALUE  )
h# 32 to  window-top  (  VALUE  )

[message]  Variables.  Should generate errors
h# 12 to  #line  (  VRBLE  )
h# 12 to  #out  (  VRBLE  )
h# 12 to  base  (  VRBLE  )
h# 12 to  mask  (  VRBLE  )
h# 12 to  span  (  VRBLE  )
h# 12 to  state  (  VRBLE  )

multi-line  #message" Using ['] on words that are both FWords and FCodes "\
                      should generate no errors"
['] new-device     drop
['] finish-device  drop
['] offset16       drop
['] instance       drop
['] end0           drop

fcode-end
