#!/bin/bash
#
#	MuonPi Data Analysis Tool - Version 1.0 - 19/04/2020
#		
#	Automated Script for plotting multiple compared detector data from MuonPi "compare_vX" software in a folder structure.
# 	Uses MuonPi "plot_muonpi_coinc.gp" Gnuplot 5 routine for automated plotting coincident 
#	events of detector pairs.   
#
#	Usage:
#
#	./auto_comp_plot.sh ARG1
#
# 	ARG1 is FOLDER containing the compared data, folder structure:
# 	FOLDER
# 	--- det10
# 	--- --- RunXdet11RunXdet10_compared.dat 
# 	--- --- RunXdet12RunXdet10_compared.dat
# 	--- --- ...
# 	--- --- RunXdetNRunXdet10_compared.dat
# 	--- det11
# 	--- ...
# 	--- detN
# 	ARG1 can also be a "detYY" folder, so also only a single detector folder can be analyzed.
#	Here, "YY" is the detector number and "X" is the run number. 
#
#	Output: Run5det11Run5det10_compared.pdf (in case of run 5, folder det10 compared with det11)
#
#	Written by Lukas Nies <lnies@muonpi.org>
#
# Provide absolute location of plot routine 
path_plot_routine=/home/lnies/muonpi/plot_muonpi_coinc.gp
#
#
echo "Current Working Directory: "; pwd
echo "ARG1 (Location of plotting routine): $1"
echo "ARG2 (Root directory of measurement run): $2"
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
					infile="${file1}${file2}_compared.dat"
					outfile="${file1}${file2}_compared.pdf"
					# echo "/home/lnies/muonpi/compare_v5 ${file2_data} ${file1_data} -o ${outfile} -e 3"
					gnuplot -c $path_plot_routine $infile $outfile 1 300 ""
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
 