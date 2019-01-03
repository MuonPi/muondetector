
f(x)  = a + b*x
a = 0.1
b = 1.

fit f(x) "ubias_scan3.txt" using 11:3 via a,b

plot "ubias_scan3.txt" u 11:3, f(x)
pause -1

