\  Test FLOADing and ENCODing-a-File with embedded Env't V'bles in pathnames
\  Let's not add any requirements on the environment.  We'll expect the
\      existence of a sibling-directory called  TokeCommon
\      and the non-existence of anything with "NonExist" in its name.


fcode-version2
hex
headers

\  Constructs like :h don't work because the path-string
\      gets interpreted by Bourne Shell syntax.
." ${PWD:h}/TokeCommon/OneBeer.fth"
fload ${PWD:h}/TokeCommon/OneBeer.fth

\  This works.
." ${PWD}/../TokeCommon/OneBeer.fth"
fload ${PWD}/../TokeCommon/OneBeer.fth

\  \  Fuggedabout these...
\  ." $PWD:h/TokeCommon/OneBeer.fth"
\  fload $PWD:h/TokeCommon/OneBeer.fth
\  fload $PWD:h/TokeCommon/NonExist.fth
\  ." ../../$PWD:h:t/TokeCommon/BinData.bin"
\  encode-file ../../$PWD:h:t/TokeCommon/BinData.bin
\  ." ../../$PWD:h:t/TokeCommon/ZeroLen.bin"
\  encode-file ../../$PWD:h:t/TokeCommon/ZeroLen.bin

\  Intended not to work
fload $PWD/../NonExist/NonExist.fth
[message] Fload an Unreadable file (No read Permissions)
fload ${PWD}/../TokeCommon/MyBeerAndYouCannotHaveIt.fth
[message] Fload a Zero-Length file (extension doesn't matter...)
fload ${PWD}/../TokeCommon/ZeroLen.bin

[message] FLoad with intentional syntax error
fload ${PWD/../TokeCommon/OneBeer.fth

\  This works.
." $PWD/../TokeCommon/BinData.bin"
encode-file $PWD/../TokeCommon/BinData.bin
" $PWD/../TokeCommon/BinData.bin" property

\  Intended not to work
[message] Encode a NonExistent file
encode-file $PWD/../NonExist/NonExist.bin
[message] Encode an Unreadable file (No read Permissions)
encode-file $PWD/../TokeCommon/NoRead.bin

[message] Encode with intentional syntax error
encode-file ${PWD/../TokeCommon/BinData.bin

[message] Is this a syntax error?  On some O/S'es but not others...
." $PWD}/../TokeCommon/BinData.bin"
encode-file $PWD}/../TokeCommon/BinData.bin

[message] Encode a Zero length data file
." $PWD/../TokeCommon/ZeroLen.bin"
encode-file $PWD/../TokeCommon/ZeroLen.bin

." That is all..."

fcode-end
