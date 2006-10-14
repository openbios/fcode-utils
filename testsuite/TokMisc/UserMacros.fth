\  Test of user-defined macros and other new features,
\      needed for compatibility with internal sources.


[macro] lookma  .( Look, Ma!  No hands! Ooooops! )
[macro] lookpa  .( Hey, Pa!  Hands! HaHa! ) 
[macro] f[lookout  f[ .( Look out, look out look out! ##Crash!# )
[macro] f[lookquote  f[ ." I'll never forget you #Leader of the Pack!#"

fcode-version2
headers

lookma
lookpa
f[lookout  ]f
f[lookquote ]f

[macro] 4+  4 +
[macro] 3+  3 +

f[  h# 800  next-fcode  ]f

: surplus
    4+ 
     3+ 
      2+
       1+
;
[message]  Now for some fun
: sourpuss
   a#   CPU
   al#  CPU
   a#   ICUP
   al#  ICUP
   a#   IPEEINACUP
   al#  IPEEINACUP
;

a#   CPU      constant a#CPU
al#  CPU      constant al#CPU
a#   ICUP     constant a#ICUP
al#  ICUP     constant al#ICUP
a#   IPEEINACUP    constant a#IPEEINACUP 
al#  IPEEINACUP    constant al#IPEEINACUP

fcode-push
f[  h# 800  next-fcode  ]f
fcode-pop


fcode-end


