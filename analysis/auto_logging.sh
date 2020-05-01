#!/bin/bash
#
#	MuonPi Data Analysis Tool - Version 1.0 - 19/04/2020
#		
#	Automated Script for plotting multiple processed log data of MuonPi detectors in a folder structure.
# 	Uses MuonPi "plot_muonpi_log.gp" Gnuplot 5 routine for automated plotting of detector logs.
#  
#	Usage:
#
#	./auto_logging.sh ARG1 
#
# 	ARG1 is FOLDER containing the data and log files, folder structure:
# 	FOLDER
# 	--- det10
# 	--- --- RunXdet10_processed.log 
# 	--- --- RunXdet10_processed.dat
# 	--- det11
# 	--- --- RunXdet11_processed.log
# 	--- --- RunXdet11_processed.dat
#	--- ...
# 	--- detN
# 	ARG1 can also be a "detYY" folder, so also only a single detector folder can be analyzed.
#	Here, "YY" is the detector number and "X" is the run number. 
#
#	Output: Run5det10_log.pdf (in case of run 5, folder det10)
#
#	Written by Lukas Nies <lnies@muonpi.org>
#
# Provide absolute location of plot routine 
path_routine=/home/lnies/muonpi/plot_muonpi_log.gp
#
echo "Current Working Directory: "; pwd
echo "ARG1: $1" # data folder
echo "ARG2: $2" # data type to process
echo "+++ Start processing +++"
for f in $1*;
 	do
 	# Check if f is a folder
 	if [ -d "$f" ]
 		then	
		cd "$f" &&
	  	echo "Enter folder ${f}"
	    # Output file name, without "/" and "." from folder path
	    out=$(echo "${f}" | sed 's=/==g' | sed 's/.//')
	    filename_log="${out}_processed_log.dat"
	    filename_data="${out}_processed_dat.dat"
		filename_out="${out}_log.pdf"
		gnuplot -c $path_routine $filename_log $filename_data $filename_out   	
		cd ../..
	else
		continue
	fi
	done;
echo "+++ Processing done +++"
 