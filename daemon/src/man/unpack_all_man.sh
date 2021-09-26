#!/bin/bash
cd $(dirname $0)
while read x; do
gzip -d $x*
done << EOF
$(ls -d */  $1)
EOF
