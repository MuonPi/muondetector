#!/usr/bin/gnuplot --persist
# gnuplot macro for plotting multiple log parameters from MuonPi shower detector log files in a multiplot with common date (x-)axis
# generates the file "output.pdf"
# usage: gnuplot -e 'filename="<filename>"' plot_muonpi_log.gp
# where <filename> is the log file name
# 30.3.2020, HG Zaunick
# zaunick@exp2.physik.uni-giessen.de

# user-adjustable parameters
# in this stringlist all the desired parameters (as they appear in the log file) are concatenated which shall be plotted in single subplots of a multiplot
paramList="timeAccuracy sats clockDrift clockBias preampNoise preampAGC geoHorAccuracy geoVertAccuracy temperature vbias ibias freqAccuracy rateXOR jammingLevel maxCNR"
# here all the parameter pairs are listed, which shall be plotted in the same plot (pairwise)
samePlotParamList="geoHorAccuracy geoVertAccuracy"
# the following list contains all parameters which shall be plotted in logarithmic scale
logPlotParamList="timeAccuracy geoHorAccuracy rateXOR"
# stepping: one data point from the log data will be drawn every N points 
stepping = 1
# max number of y-axis tics
nytics = 9

# set output type to pdf, DIN A3
set terminal pdf color fontscale 0.35 size 42.0cm,29.7cm
# set output file name
set output "output.pdf"

# helper function for setting the axis increment reasonably
endsinone(n) = strstrt(gprintf("%g", n), "1")
getincr(range, maxincr, guess) = range/guess < maxincr ? guess : \
    (endsinone(guess) ? getincr(range, maxincr, 5*guess) : getincr(range, maxincr, 2*guess))

# set number of parameters and plots from the count of parameters supplied in paramList
nparams=words(paramList)
nplots=words(paramList)-words(samePlotParamList)/2

# print out the input file name
print "filename = ", filename;

# the following two lines are just for counting the lines in the log file for fun
line0 = filename
stats line0 using 1 name 'datestats' # nooutput

#set some plot specific things
set title filename
#set pointtype 7
set pointsize 0.2
lineWidth=10
mainPointColor=8
secondPointColor=12

# set up the major and minor grid lines
set style line 100 lt 0.5 lc rgb "gray" lw 5
set style line 101 lt 0.5 lc rgb "gray" lw 2
set grid mytics ytics ls 100, ls 101
# height holds the height of each plot in relative coordinates, leave 5% margin on top and bottom
height=0.9/nplots

# this variable holds the current (upper) y-position of the currently drawn plot
currPlotPointer=0.96

# set up multiplot with N rows
set multiplot layout nplots,1 rowsfirst
# unset xlabels for now since the x-axis should be labeled only once for the lowermost plot
unset xlabel
set format x ""

# loop over all plots
#do for [i=1:N] {
iparam=1;
iplot=1;
while (iparam<=nparams && iplot<=nplots) {
  # have to explicitly unset the time mode for x axis since stats work only in normal mode
  set xdata
  unset label
  sameParams=0;
  do for [j=1:words(samePlotParamList)] {
    if (word(paramList,iparam) eq word(samePlotParamList,j)) {
      print "same params detected: ".word(paramList,iparam)." and ".word(paramList,iparam+1)
      sameParams=1;
    }
  }
  print "same=".sameParams
  # generate and print the string with the filter for the desired parameter (bash command insertion)
  line="<cat ".filename." | grep '".word(paramList,iparam)."'"
  print line
  unitStrCmd="cat ".filename." | grep '".word(paramList,iparam)."' | head -n1 | awk '{print $4}'"
  unitStr=system(unitStrCmd)
  print "unit=".unitStr
  # generate stats for this parameter and store it in variable 'stat'
  print "*** Stats for ".word(paramList,iparam)." ***"
  stats line using 3 name 'stat' # nooutput

  # generate and print the string with the filter for the desired second parameter which should go into the same plot
  secondLine=""
  if (sameParams==1) {
    secondLine="<cat ".filename." | grep '".word(paramList,iparam+1)."'"
    print secondLine
    # generate stats for this parameter and store it in variable 'stat2'
    print "*** Stats for ".word(paramList,iparam+1)." ***"
    stats secondLine using 3 name 'stat2' # nooutput
  }

  # set time mode for x axis and log time format
  set xdata time
  set timefmt "%Y-%m-%d_%H-%M-%S"
  
  # switch on x-axis labels for lowermost plot only
  if (iplot==nplots) {
    set xlabel
    set format x "%d/%m/%y\n%Hh"
  }

  # sets the position of the current plot
  set tmargin at screen currPlotPointer; set bmargin at screen currPlotPointer-height
  set lmargin at screen 0.05; set rmargin at screen 0.95

  # look if the current par should be plotted in log scale
  isLogScale=0
  do for [j=1:words(logPlotParamList)] {
    if (word(paramList,iparam) eq word(logPlotParamList,j)) {
      isLogScale=1
    }
  }
    
  # set logscale for selected parameter plots only
  if (isLogScale==1) {
      set logscale y
      set logscale y2
      unset ytics
      unset y2tics
      set ytics
      set y2tics
  } else {
      # set the number of y tics to the nytics variable
      # this is tricky in gnuplot, solved here with the getincr helper function
      # note: use the first line for normal ticmarks: with this setting the y axis labels of adjacent plots usually overlap -> ugly
      # use the second line to limit the range, where tics are shown to the range where data actually exists, which looks nicer
      # in some plots there may be too few tics, though
      #set ytics getincr(stat_max-stat_min, nytics, 1e-9)
      #set ytics getincr(stat_max-stat_min, nytics, 1e-9) rangelimited
      # forget everythin' above. this simple setting seems to work best 
      set ytics rangelimited
      set y2tics
      #set ytics
  }
  
  set ylabel unitStr
  # do the plot
  if (sameParams==1) {
      set label word(paramList,iparam) at graph 0.01,0.88 left offset character 1,-0.25 point pt 0 lc mainPointColor
      set label word(paramList,iparam+1) at graph 0.01,0.75 left offset character 1,-0.25 point pt 0 lc secondPointColor
      set label word(paramList,iparam) at graph 0.99,0.88 right offset character -1,-0.25 point pt 0 lc mainPointColor
      set label word(paramList,iparam+1) at graph 0.99,0.75 right offset character -1,-0.25 point pt 0 lc secondPointColor
      plot line u 1:3 every stepping w points pt 7 lc mainPointColor notitle, \
           secondLine u 1:3 every stepping w points pt 7 lc secondPointColor notitle
  } else {
      set label word(paramList,iparam) at graph 0.01,0.88 left offset character 1,-0.05 point pt 0 lc mainPointColor
      set label word(paramList,iparam) at graph 0.99,0.88 right offset character -1,-0.05 point pt 0 lc mainPointColor
      plot line u 1:3 every stepping with points pt 7 lc mainPointColor notitle
  }
  
  # if logscale is set, unset it again
  unset logscale y
  unset logscale y2
  # unset the plot title such that it appears only above the uppermost plot
  unset title
  if (sameParams==1) { 
    iparam=iparam+1;
  }
  iplot=iplot+1;
  # increase the position pointer for the next loop
  currPlotPointer=currPlotPointer-height
  iparam=iparam+1;
  print "iparam=".iparam
  print "iplot=".iplot
}

unset multiplot
