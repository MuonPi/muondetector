#!/bin/bash -e
objdump -p $1 | grep NEEDED | awk '{print $2}' > dependencies.tmp
wait
while read p; do
dpkg -S $p | awk '{print $1}' > packages.tmp
done < dependencies.tmp
wait
cat packages.tmp | sed -n '/-dev/p' > packages-dev.tmp
