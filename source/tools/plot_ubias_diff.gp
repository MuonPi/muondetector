
f(x)  = a + b*x
a = 0.1
b = 1.

fit f(x) "ubias_scan3.txt" using 11:($9 -$11) via a,b

plot "ubias_scan3.txt" u 11:($9 -$11), f(x)

pause -1

