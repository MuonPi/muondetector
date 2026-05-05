#!/bin/bash
cd $(dirname $0)
while read x; do
gzip $x*
done << EOF
$(ls -d */  $1)
EOF
