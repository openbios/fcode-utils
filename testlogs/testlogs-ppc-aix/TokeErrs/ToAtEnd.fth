\  Confirm that I now catch the error where
\     "to"  is the last thing in the input

\  Updated Mon, 03 Apr 2006 at 16:03 PDT by David L. Paktor

fcode-version2
headers
global-definitions
alias poo to
device-definitions

\  We're also going to throw in a quickie test for how we handle
\      attempts to create an alias to a Local.
[flag] Local-Values
fload LocalValuesSupport.fth

:  gnarggghhh!  { _gnarly _dood | _hang_ten }
    alias _cool _gnarly
  _cool _dood + -> _hang_ten
;
_cool
   _gnarly
      _dood 
          _hang_ten
variable shmoo
\  What's the world coming to?
h# 5417 value merde
h# 4ead poo
     merde
h# f09e4ead poo gnarggghhh!
h# f09e4ead to gnarggghhh!
h# f09e4ead poo shmoo
h# f09e4ead poo 1
merde poo
