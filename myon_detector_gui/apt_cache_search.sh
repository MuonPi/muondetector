#!/bin/bash
rm dependencies.tmp 2> /dev/null
ldd $1 | awk '{print $1}' | tr '[:upper:]' '[:lower:]' | sed 's/.so.//g' >> dependencies.tmp
rm results.tmp 2> /dev/null
wait
while read p; do
apt-cache search $p | grep -w $p | awk '{print $1}' >> results.tmp
done < dependencies.tmp
#wait
#while read q; do
#sed -i.bak '/$q"-"/d' results.tmp
#done < dependencies.tmp
