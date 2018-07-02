#!/bin/bash

# measurement interval in s
MEAS_INTERVAL=30

ts_start=`date "+%s"`
#echo $ts_start
dt=0
counter=0

while (( $dt < MEAS_INTERVAL ))
do
 gpio -g wfi 6 rising
 counter=$(($counter+1))
 # get current timestamp
 ts_curr=`date "+%s"`
 dt=$((ts_curr-ts_start))
 #echo $dt
 #sleep 1
done


counts_per_s=$(echo "scale=2; $counter/$MEAS_INTERVAL" | bc)
echo $counter "counts = " $counts_per_s "cts/s"
#gpio -g wfi 6 rising
