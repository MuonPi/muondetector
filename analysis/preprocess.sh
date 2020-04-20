#!/bin/bash
#
#	MuonPi Data Analysis Tool - Version 1.0 - 19/04/2020
#		
#	Automated Script for combining raw data and log files into a single 
#	data and log file in a folder structure for later processing. 
#       
#	Usage:
#
#	./auto_compare.sh ARG1 ARG2
#
# 	ARG1 is FOLDER containing the data files, folder structure:
# 	FOLDER
# 	--- det10
# 	--- --- data_2020-04-16_19-46-16.dat
# 	--- --- data_2020-04-16_11-11-23.log
#   --- --- ...
# 	--- det11
# 	--- --- data_2020-04-16_19-34-13.dat
#	--- ...
# 	--- detN
# 	ARG1 can also be a "detYY" folder, so also only a single detector folder can be analyzed.
#	Here, "YY" is the detector number and "X" is the run number.
#
#	ARG2 {"dat", "log", "all"}: Argument declaring the type of file to be processed:
#	- "dat": processes data files
#	- "log": processes log files
#	- "all": processes both file types
#
#	WARNING: Already existing ..._processed.dat files are replaced by new processed files.
#
#	Output: Run5det10_processed.dat (in case of run 5, folder det10)
# 
#	Written by Lukas Nies <lnies@muonpi.org>
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
	    # check if processed data already exist, if yes, remove (removes all non-data/log files from folder!)
		for d in ./*
	    do
	    	#echo "d = ${d}"
			if [[ $d == './data'*  ]] || [[ $d == './log'* ]]
				then
				continue
			elif [[ $d == './Run'*'.dat'  ]]
				then
				echo "Found already pre-processed file: ${d}"
				rm $d # dangerous! Already deleted important stuff during debugging :D
		    fi
		done;
	    # Only process data
	    if [ $2 == 'data' ]
	   	    then
	   	    echo "process data files..."
		    for d in ./data*
		    	do
		     	sed -i '1d' "$d"
		     	cat $d >> "${out}_processed_dat.dat"
		    	done;
		# Only process log files
		elif [ $2 == 'log' ]
		    then
		    echo "process log files..."
		    for d in ./log*
		    	do
		     	sed -i '1d' "$d"
		     	cat $d >> "${out}_processed_log.dat"
		    	done;
		# Process both
		elif [ $2 == 'all' ]
		    then
		    echo "process log files..."
		    for d in ./log*
		    	do
		     	sed -i '1d' "$d"
		     	cat $d >> "${out}_processed_log.dat"
		    	done;
		    echo "process dat files..."	
		    for d in ./data*
		    	do
		     	sed -i '1d' "$d"
		     	cat $d >> "${out}_processed_dat.dat"
		    	done;
		else
			echo 'processor type unclear, check ARG2'
		fi
		cd ../..
	else
		continue
	fi
	done;
echo "+++ Processing done +++"
 