#!/bin/bash
# a script that searcehs a gievn directory on the fgilesytstem for a given text file
#task1 returns value 1 error if parametners not specificied
#task2 returnbs value1 error if given directory doesnt match any on file system
#task 3prints X files and Y strings of matching 

#check if all inputs were given
if [ "$#" -ne 2 ]; then
	echo "Parameters not specified"
	exit 1
fi
#capture the inputs
filesdir=$1
searchstr=$2

#getting directory read errors due to input
#filesdir=${filesdir%/}
#echo "filesdir: $filesdir" #terminal check for directory validity

#Error if given directory is not valid 
if [ ! -d $filesdir ]; then
	echo "Error: Not a directory: $filesdir" 
	exit 1
fi

#give counts of matches for files 
X=$(find "$filesdir" -type f | wc -l)
 echo "Files in #filesdir is $X "
 
# Search for the given string in each file within the directory
Y=$(find "$filesdir" -type f -exec cat {} + | grep -c "$searchstr")	

echo The number of files are $X and the number of matching lines are $Y





