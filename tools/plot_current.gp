
set yrange [0.00001:500]
set logscale y

plot "ubias_scan_SIPM.txt" u 11:((($9 -$11)-(-0.0115-0.004639*$11))*100)

pause -1

