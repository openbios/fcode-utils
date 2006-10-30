\  Test FLOADing and ENCODing-a-File with an Include-List
\  We'll expect the Include-List to include the sibling-directory
\      called  TokeCommon  and the non-existence of anything with
\      "NonExist" in its name.


fcode-version2
hex
headers

\  This works.
." OneBeer.fth"
fload OneBeer.fth

\  Intended not to work
[message] Fload a NonExistent file
fload NonExist.fth
[message] Fload an Unreadable file (No read Permissions)
fload MyBeerAndYouCannotHaveIt.fth
[message] Fload a Zero-Length file (extension doesn't matter...)
fload ZeroLen.bin

\  This works.
." BinData.bin"
encode-file BinData.bin
" BinData.bin" property

\  Intended not to work
[message] Encode a NonExistent file
encode-file NonExist.bin
[message] Encode an Unreadable file (No read Permissions)
encode-file NoRead.bin

[message] Encode with intentional syntax error
encode-file ${PWD/../TokeCommon/BinData.bin

[message] Encode a Zero length data file
." ZeroLen.bin"
encode-file ZeroLen.bin

." That is all..."

fcode-end
