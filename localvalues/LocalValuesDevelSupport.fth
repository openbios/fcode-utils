\       (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
\       Licensed under the Common Public License (CPL) version 1.0
\       for full details see:
\            http://www.opensource.org/licenses/cpl1.0.php
\
\       Module Author:  David L. Paktor    dlpaktor@us.ibm.com

\  Support file for development of FCode that uses Local Values
\  FLoad this right after  LocalValuesSupport.fth
\  Remove it from your final product.

\  Exported Function:  max-local-storage-size  ( -- n )
\      Returns the measured maximum size of storage for Local Values
\      used by any given test run.  This number can be used to guide
\      the declaration of  _local-storage-size_ 
\
\       (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
\       Module Author:  David L. Paktor    dlpaktor@us.ibm.com

\  Count the current depth on a per-instance basis, 
\  but collect the maximum depth over all instances.

headers
0 instance value local-storage-depth

external
0 value max-local-storage-size
headers

\  Overload the  {push-locals}  and  {pop-locals}  routines to do this.
\  Do not suppress the overload warnings; they'll serve as a reminder.
: {pop-locals} ( #locals -- )
    local-storage-depth over - to local-storage-depth
    {pop-locals}
;

: {push-locals} ( #ilocals #ulocals -- )
   2dup + local-storage-depth +
   dup to local-storage-depth
   max-local-storage-size max
   to max-local-storage-size
   {push-locals}
;


