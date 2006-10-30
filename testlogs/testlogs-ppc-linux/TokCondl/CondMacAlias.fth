\  Test Conditionals and Locals with Macros and Aliases
\      without (intentional) errors, this time...
\  Updated Fri, 10 Mar 2006 at 14:26 PST by David L. Paktor

headers
fcode-version2

fload GlobalLocalValues.fth

Global-definitions

alias  // \
alias  (((  (

alias loc{  {

alias   [whatden]  [endif]
alias   [younwhudahmy]  [else]
[macro]  [donewidit]  [whatden]
[macro]  [udderwise]  [younwhudahmy]


alias [isitdere]    [ifexist]
alias [ifitaint]    [ifnexist]
[macro] doIgotit    [isitdere]
[macro] Iaintgotit  [ifitaint]

[macro] [ifitis] Iaintgotit  gumblefritz

device-definitions

\  Don't define  gumblefritz

[message]  Didn't define  gumblefritz

[ifitaint]  gumblefritz
   #message"  Can't ignore  fload.  What if it's got a balancing [endif] ?"
   [message] Or a balancing  [else]
   fload CondMacAlias_01.fth
[udderwise]
   #message"  Dis ain't gonna show even if it has an [endif] "
   [message] Dere's an  [endif]  here too
[donewidit]

new-device

create gumblefritz
[message]  Just defined  gumblefritz

doIgotit  gumblefritz

    fload CondMacAlias_01.fth

[donewidit]

finish-device

fcode-end
