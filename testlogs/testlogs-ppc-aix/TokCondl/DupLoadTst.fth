\  Test for being able to control duplicate loading of a source file
\  Updated Thu, 27 Jul 2006 at 15:23 PDT by David L. Paktor


fcode-version2

."  Going once," cr

fload DupLoadBody.fth

."  Going twice." cr

fload DupLoadBody.fth

."  Gone!" cr

fcode-end
