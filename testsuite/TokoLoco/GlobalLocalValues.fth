\  Load Locals Support under Global-Definitions.  Bypass Instance warning

\  Updated Fri, 10 Mar 2006 at 14:47 PST by David L. Paktor

\  Make sure this option is turned on.
[flag] Local-Values

global-definitions

\  Bypass warning about Instance without altering LocalValuesSupport file
alias generic-instance  instance
[macro] bypass-instance  f[  noop  .( Bypassed instance!) f]

overload alias instance bypass-instance

fload LocalValuesSupport.fth

\  Replace normal meaning of  Instance, still in Global scope.
overload alias instance generic-instance

\  Restore Device-Definitions scope.
device-definitions
