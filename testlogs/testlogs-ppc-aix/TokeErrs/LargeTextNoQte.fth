\  We tested what happens when a quote or message or string is
\      not terminated until many lines later, but what happens
\      when it is not terminated at all and the text until the
\      end of file exceeds the allowable buffer size?
\  FLOAD this file into a master, to test the overall effect.
\      The master must define a Macro or Alias called  test-token  
\      which will supply the action for the variant in question.

\  Updated Tue, 09 May 2006 at 09:13 PDT by David L. Paktor


test-token         \  Okay, kids, here we go!

    Oh, ten Bottles of Beer on the wall, cr
	ten Bottles of Beer!, cr
    Take one down and pass it around, cr
    nine Bottles of Beer on the wall. cr
    Oh, nine Bottles of Beer on the wall, cr
	nine Bottles of Beer!, cr
    Take one down and pass it around, cr
    eight Bottles of Beer on the wall. cr
    Oh, eight Bottles of Beer on the wall, cr
	eight Bottles of Beer!, cr
    Take one down and pass it around, cr
    seven Bottles of Beer on the wall. cr
    Oh, seven Bottles of Beer on the wall, cr
	seven Bottles of Beer!, cr
    Take one down and pass it around, cr
    six Bottles of Beer on the wall. cr
    Oh, six Bottles of Beer on the wall, cr
	six Bottles of Beer!, cr
    Take one down and pass it around, cr
    five Bottles of Beer on the wall. cr
    Oh, five Bottles of Beer on the wall, cr
	five Bottles of Beer!, cr
    Take one down and pass it around, cr
    four Bottles of Beer on the wall. cr
    Oh, four Bottles of Beer on the wall, cr
	four Bottles of Beer!, cr
    Take one down and pass it around, cr
    three Bottles of Beer on the wall. cr
    Oh, three Bottles of Beer on the wall, cr
	three Bottles of Beer!, cr
    Take one down and pass it around, cr
    two Bottles of Beer on the wall. cr
    Oh, two Bottles of Beer on the wall, cr
	two Bottles of Beer!, cr
    Take one down and pass it around, cr
    one Bottle of Beer on the wall. cr
    Oh, one Bottle of Beer on the wall, cr
	one Bottle of Beer!, cr
    Take it down and pass it around... cr
    How dry I am!  How dry I am! cr
    No body knows how dry I am. cr
    How ... Dry I ... Aaaammm.  cr
