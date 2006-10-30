\  %Z%%M% %I% %W% %G% %U%
\       (C) Copyright 2005-2006 IBM Corporation.  All Rights Reserved.
\       Licensed under the Common Public License (CPL) version 1.0
\       for full details see:
\            http://www.opensource.org/licenses/cpl1.0.php
\
\       Module Author:  David L. Paktor    dlpaktor@us.ibm.com

\  Control file for loading of Local Values Support file with variants.
\  Command-line Symbol-definitions select whether the support will
\      be under Global-Definitions, and whether to include the extra
\      Development-time support features.
\
\  The command-line symbols are:
\            Global-Locals
\      and
\            Locals-Release
\
\  The default is device-node-specific support in a Development-time setting.
\
\  If  Global-Locals  is defined, support will be under Global-Definitions
\  If  Locals-Release  is defined, this is a final production release run,
\      and the Development-time support features will be removed.

\  Make sure this option is turned on.
[flag] Local-Values

[ifdef] Global-Locals
    \  Load Support file under Global-Definitions.
    global-definitions

    \  Bypass warning about Instance without altering LocalValuesSupport file
    alias generic-instance  instance
    [macro] bypass-instance  f[  noop  .( Bypassed instance!) f]

    overload alias instance bypass-instance
[endif]  \  Global-Locals

fload LocalValuesSupport.fth

[ifndef] Locals-Release
    \  Load Development-time support features
    fload LocalValuesDevelSupport.fth
[endif]  \  not Locals-Release

[ifdef] Global-Locals
    \  Replace normal meaning of  Instance, still in Global scope.
    overload alias instance generic-instance

    \  Restore Device-Definitions scope.
    device-definitions
[endif]  \  Global-Locals
