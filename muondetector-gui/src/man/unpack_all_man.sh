#!/bin/bash
while read x; do
gzip -d $x*
done << EOF
$(ls -d */  $1)
EOF
