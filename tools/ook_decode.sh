#!/bin/bash

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

rtl_433 -f 433.95M -s 1024k -R 0 -F json -M bits \
  -X 'n=muonpi,m=OOK_MC_ZEROBIT,s=500,l=0,r=5000,match={40}555555552d,unique' \
  | "${SCRIPT_DIR}/ook_decode_rtl433.py" --raw
