#!/bin/bash
while read x; do
gzip $x*
done << EOF
$(ls -d */  $1)
EOF
