#!/usr/bin/gnuplot --persist
#
#	MuonPi Data Analysis Tool - Version 1.0 - 19/04/2020
#		
#	Automated Gnuplot 5 Script for plotting compared detector data from MuonPi "compare_vX" software.
#
#	Usage:
#
#	gnuplot -c auto_comp_plot.sh ARG1 ARG2 ARG3 ARG4
#
# 	ARG1: Path for the data file. Should be in form of the MuonPi "compare_vX" software output (Third column: UNIX Timestamp, Fourth column: time difference [ns])
# 	ARG2: Path for the output.
# 	ARG3: Binwidth for the time difference binning. Standard 1ns (resolution of the UNIX timestamps)
# 	ARG4: Binwidth for the event rate binning. Standard 100s (Events per second, averaged over 100s)
# 	ARG5: Number of bins for the moving average filter of the event plotting. Standard 100 bins, if empty (""), not applied 
#
#	Output: Three plots:
#	--- 1. Time difference distribution with Gaussian Fit
#	--- 2. Event rate histogram with average drawn in
#   --- 3. Events versus event-time. Can be smoothed with Moving Average Filter (a little buggy)
#
#	Written by Lukas Nies <lnies@muonpi.org>, visit MuonPi.org for more information.

# store and print out the command line arguments 
if ( ARG1 eq "" ){ filename_data = 'dummy_data.txt' # Standard log data file name
} else { filename_data = ARG1 }
if ( ARG2 eq "" ){ filename_out = 'out.pdf' # Standard log data file name
} else { filename_out = ARG2 }
if ( ARG3 eq "" ){ binwidth = 1 # 
} else { binwidth = ARG3 }
if ( ARG4 eq "" ){ binwidth = 100 # 
} else { binwidth_rate = ARG4 }
if ( ARG5 eq "" ){ n = 100 # 
} else { n = ARG5 }
print "(ARG0) SCRIPT_NAME = ", ARG0; 
print "(ARG1) data filename = ", filename_data;
print "(ARG2) output filename = ", filename_out;
print "(ARG3) Bin width time difference [ns] = ", binwidth;
print "(ARG4) Bin width count rate [s] = ", binwidth_rate;
print "(ARG5) Averaging interval [#events] = ", n;
# set output type to pdf
set terminal pdf color fontscale 0.2
# set uotput file name
set output filename_out
#### Moving average caluclation taken from https://stackoverflow.com/questions/42855285/plotting-average-curve-for-points-in-gnuplot?answertab=votes#tab-top
# initialize the variables
do for [i=1:n] {
    eval(sprintf("back%d=0", i))
}
# build shift function (back_n = back_n-1, ..., back1=x)
shift = "("
do for [i=n:2:-1] {
    shift = sprintf("%sback%d = back%d, ", shift, i, i-1)
} 
shift = shift."back1 = x)"
# uncomment the next line for a check
# print shift
# build sum function (back1 + ... + backn)
sum = "(back1"
do for [i=2:n] {
    sum = sprintf("%s+back%d", sum, i)
}
sum = sum.")"
# uncomment the next line for a check
# print sum
# define the functions like in the gnuplot demo
# use macro expansion for turning the strings into real functions
samples(x) = $0 > (n-1) ? n : ($0+1)
avg_n(x) = (shift_n(x), @sum/samples($0))
shift_n(x) = @shift
####
#
# Histogram function
bin(x,width)=width*floor(x/width) + width/2.0
set boxwidth binwidth
# Set formats for the first histogram
set style data histogram 
set style histogram
set style fill solid border 2
set xrange [-100:100] # in ns
set grid
# Save the first histogram in a temporary table for fitting
set table 'table_temp.dat'
plot filename_data \
	using (bin($4,binwidth)):(1.0) smooth freq with boxes \
	lc rgb '#1A6FDF' title "All Events"
unset table
# Setup Gaussian fit
A = 10
mean = -10
sigma = 5
gauss(x)= A * (sigma*sqrt(2.*pi))*exp(-(x-mean)**2/(2.*sigma**2))
fit gauss(x) "table_temp.dat" u 1:2 via A, sigma, mean
####
#### Set up multiplot with 3 rows
####
set multiplot layout 3,1 rowsfirst
set title filename_data
###
### First Plot: Histogram for time differences
###
# Print fit parameters, 0,0 is bottom left and 1,1 is top right.
x_position = 0.05
y_position = 0.9
y_position_line_break = 0.08
set label 1 at graph x_position, y_position front left
set label 1 sprintf('Fit parameters:') tc "black" font "Verdana, 16"
set label 2 at graph x_position, y_position - 1 * y_position_line_break front
set label 2 sprintf('A = %3.3f',A) tc "black" font "Verdana, 16"
set label 3 at graph x_position, y_position - 2 * y_position_line_break front
set label 3 sprintf('Mean = %3.3f ns',mean) tc "black" font "Verdana, 16"
set label 4 at graph x_position, y_position - 3 * y_position_line_break front
set label 4 sprintf('Sigma = %3.3f ns',sigma) tc "black" font "Verdana, 16"
set label 5 at graph x_position, y_position - 4 * y_position_line_break front
set label 5 sprintf('FWHM = %3.3f ns',2.3548*sigma) tc "black" font "Verdana, 16"
set obj 10 rect at graph x_position,y_position front size char strlen('Fit parameters:')+1, char 1.5 fc rgb "white"
# Set x and y label
set xlabel "Time difference [ns]"
set ylabel "Counts"
# Plot Histogram
plot filename_data \
	using (bin($4,binwidth)):(1.0) smooth freq with boxes \
	lc rgb '#1A6FDF' title "Coincident Events", \
    gauss(x) lc rgb '#F14040' lw 3 title "Gaussian Fit"
# Unset all styles etc
unset xrange
unset style data 
unset style
unset label 1; unset label 2; unset label 3; unset label 4; unset label 5
unset obj 10
###
### Second plot: Histogram for count rate
###
# Simple data evaluation
set table 'table_temp_2.dat'
plot filename_data \
	using (bin($3,binwidth_rate)):(1.0/binwidth_rate) smooth freq with boxes \
	lc rgb '#1A6FDF' title "All Events"
unset table
stats "table_temp_2.dat" u 2 name 'stat' nooutput
m = 0
B = stat_mean
lin(x) = m * x + B
# Print fit parameters, 0,0 is bottom left and 1,1 is top right.
x_position = 0.05
y_position = 0.9
y_position_line_break = 0.08
set label 1 at graph x_position, y_position front
set label 1 sprintf('Fit parameters:') tc "black" font "Verdana, 16"
set label 2 at graph x_position, y_position - 1 * y_position_line_break front
set label 2 sprintf('Mean = %3.3f',stat_mean) tc "black" font "Verdana, 16"
# Formatting
set xdata time
set timefmt "%s"
set xlabel
set format x "%d/%m/%y\n%H:%M"
set boxwidth 0.9
set grid
set xlabel "Time stamp [UTC]"
set ylabel "Coincident event rate [events/s]"
set style data histogram 
set style histogram
set style fill solid border 2
set boxwidth binwidth_rate
# Event rate against time
plot filename_data \
	using (bin($3,binwidth_rate)):(1.0/binwidth_rate) smooth freq with boxes \
	lc rgb '#1A6FDF' title "All Events", \
    lin(x) lc rgb '#F14040' lw 3 title "Linear Fit"
#
unset style data 
unset style
unset label 1; unset label 2; 
###
### Third plot: Histogram for count rate
###
set yrange [-100:100]
set xlabel "Time stamp [UTC]"
set ylabel "Time difference [ns]"
set style line 1 lc rgb 'black' pt 5   # square
# If moving average is not given:
if ( ARG5 eq "" ){ 
	plot filename_data \
		using 3:4 w p ls 0.5 \
		lc rgb '#1A6FDF' title "Coincident Events" 
}
else {
	plot filename_data \
		using 3:4 w p ls 0.5 \
		lc rgb '#1A6FDF' title "Coincident Events", \
		filename_data using 3:(avg_n($4)) w l lc rgb "red" lw 1 title "avg\\_".n."\\_events"
}
#
# End of plotting, do clean up
# Remove temporary table file 
system("rm table_temp.dat")
system("rm table_temp_2.dat")