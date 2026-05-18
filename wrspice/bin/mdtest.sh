#! /bin/bash

cmdstr="-l 10pH \
-c 1.0pF \
-r 0.1 \
-g 1000 \
-k 0.2 \
-x 0.1pF \
-o mysubc
-n 2
-L 1"

ycmdstr="-l10 \
-c1.0 \
-r0.1 \
-g1000 \
-k0.2 \
-x0.1 \
-omysubc
-n2
-L1"

zcmdstr="-n4 -l9e-9 -c20e-12 -r5.3 -x5e-12 -k0.7 -otest -L5.4"
cmdstr="-n 4 -l 9e-9 -c 20e-12 -r 5.3 -x 5e-12 -k 0.7 -o test -L 5.4"

./multidec $cmdstr
