\  Body 2 for test of string with c-string-escaped hex byte that ends abruptly.
\  This ends with no hex value after the last backslash, and a new-line.
\
\  Body 3 is made from this by using dd to truncate the new-line.
\  The c-shell command sequence goes like this:
\  set len = `cat CStrEscOvflw_body_2.fth | wc -c`
\  @ len--
\  dd if=CStrEscOvflw_body_2.fth of=CStrEscOvflw_body_3.fth count=1 bs=$len

." What's on s\e\ \c0\n\db\\a5\\
