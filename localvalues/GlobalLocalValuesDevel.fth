\       (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
\       Licensed under the Common Public License (CPL) version 1.0
\       for full details see:
\            http://www.opensource.org/licenses/cpl1.0.php
\
\       Module Author:  David L. Paktor    dlpaktor@us.ibm.com

\  Load Support file for development of FCode that uses Local Values
\      under Global-Definitions.  Bypass Instance warning.
\  Replace this with GlobalLocalValues.fth  in your final product.

\  Make sure this option is turned on.
[flag] Local-Values

global-definitions

\  Bypass warning about Instance without altering LocalValuesSupport file
alias generic-instance  instance
[macro] bypass-instance  f[  noop  .( Bypassed instance!) f]

overload alias instance bypass-instance

fload LocalValuesSupport.fth
fload LocalValuesDevelSupport.fth

\  Replace normal meaning of  Instance, still in Global scope.
overload alias instance generic-instance

\  Restore Device-Definitions scope.
device-definitions
