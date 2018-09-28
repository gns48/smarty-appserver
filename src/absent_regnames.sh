#!/bin/sh
for rn in `./find_regnames.sh`
   do if ! grep "$rn=" ../conf/regexlib.dat ; then echo $rn ; fi  
done | grep -v =
