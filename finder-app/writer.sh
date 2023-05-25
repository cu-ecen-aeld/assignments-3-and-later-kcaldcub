#!/bin/bash
#
#check if all inputs were given
if [ "$#" -ne 2 ]; then
	echo "Parameters not specified"
	exit 1
fi
#capture the inputs
writefile=$1
writestr=$2

#if directory does not already exist
if [ ! -d $(dirname "$writefile") ]; then
	mkdir -p $(dirname "$writefile")
fi

# Write string to ile
echo "$writestr" > "$writefile"

exit 0


