#!/bin/bash
#
#	MuonPi Data Analysis Tool - Version 1.0 - 19/04/2020
#		
#	Automated Script for comparing detector data from MuonPi detectors for coincident events using a folder structure.
# 	Uses MuonPi "compare_vX.c++" C++ routine for automated event matching of groups of detectors.    
#
#	Usage:
#
#	./auto_compare.sh ARG1 
#
# 	ARG1 is FOLDER containing the data files, folder structure:
# 	FOLDER
# 	--- det10
# 	--- --- RunXdet10_processed.dat
# 	--- det11
# 	--- --- RunXdet11_processed.dat
#	--- ...
# 	--- detN
# 	ARG1 can also be a "detYY" folder, so also only a single detector folder can be analyzed.
#	Here, "YY" is the detector number and "X" is the run number. 
#
#	Output: Run5det11Run5det10_compared.dat (in case of run 5, folder det10 compared with det11)
#
#	Written by Lukas Nies <lnies@muonpi.org>
#
# Provide absolute location of plot routine 
path_plot_routine=/home/lnies/muonpi/compare_v5.gp
#
#
echo "Current Working Directory: "; pwd
echo "ARG1: $1" # data folder
echo "ARG2: $2" # data type to process
echo "+++ Start processing +++"
# Store working directory
path=$(pwd)
# Loop through all files and folders
for f in $1*;
 	do
 	# Check if f is a folder
 	if [ -d "$f" ]
 		then	
		cd "$f" &&
	  	# echo "Enter folder ${f}"
	    # Output file name, without "/" and "." from folder path
	    file1=$(echo "${f}" | sed 's=/==g' | sed 's/.//')
	    folder1=$(echo "${f}" | sed 's/.//')
    	file1_data=$(echo "${path}${folder1}/${file1}_processed_dat.dat")
		cd ../..
		for t in $1*;
			do
			# Check if f is a folder
			if [ -d "$t" ]
			 	then	
				cd "$t" &&
				# echo "--->Enter folder ${t}"
				# Output file name, without "/" and "." from folder path
			    file2=$(echo "${t}" | sed 's=/==g' | sed 's/.//')
			    folder2=$(echo "${t}" | sed 's/.//')
		    	file2_data=$(echo "${path}${folder2}/${file2}_processed_dat.dat")
				if [[ $file1 != $file2 ]]
				    then
					echo "--->compare: ${file1} + ${file2}"
					outfile="${file1}${file2}_compared.dat"
					# echo "/home/lnies/muonpi/compare_v5 ${file2_data} ${file1_data} -o ${outfile} -e 3"
					/home/lnies/muonpi/compare_v5 $file1_data $file2_data -o $outfile -e 3
				fi
			else
				continue
			fi
			cd ../..
		    done;
		#cd ../..
	else
		continue
	fi
	done;
echo "+++ Processing done +++"
 