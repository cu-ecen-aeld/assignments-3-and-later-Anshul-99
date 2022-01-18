#!/bin/bash

# Script for Assignment 1 AESD. It accepts two arguments, a path to directory/file (filesdir) and a text string (searchstr). This script searches for searchstr in all the files present in filesdir. 
#Author: Anshul Somani

# References: https://ryanstutorials.net/bash-scripting-tutorial/bash-variables.php
# https://ryanstutorials.net/bash-scripting-tutorial/bash-if-statements.php
# https://acloudguru.com/blog/engineering/conditions-in-bash-scripting-if-statements
# https://unix.stackexchange.com/questions/6648/number-of-files-containing-a-given-string
# https://devconnected.com/how-to-check-if-file-or-directory-exists-in-bash/

arguments=$#
filesdir=$1
searchstr=$2
# Go to Home directory
cd
# Check if the number of arguments is less than 2
if [ $arguments -lt 2 ]
 then
   echo "Error: Insufficient number of parameters."
   echo "There should be a total of 2 commands. The first command is the directory to search in and the second command should be the string to search for."
   echo " ./finder.sh <path> <string>"
   exit 1
# Check if the number of arguments is greater than 2
elif [ $arguments -gt 2 ]
 then
   echo "Error: Too many parameters."
   echo "There should be a total of 2 commands. The first command is the directory to search in and the second command should be the string to search for."
   echo " ./finder.sh <path> <string>"
   exit 1
# Check if the first argument is an actual file
elif [ -f "${filesdir}" ]
 then
# Go to the file/directory mentioned in argument 1
   cd $filesdir
   # Count the number of files in the directory
   numfiles=$(grep -r $searchstr * | wc -l)
   # Count the number of times the string has been mentioned
   numlines=$(grep -r -o -h $searchstr * | wc -l)
   echo "The number of files are $numfiles and the number of matching lines are $numlines"
   exit 0
# Check if the first argument is an actual directory
elif [ -d "${filesdir}" ]
 then
 # Go to the file/directory mentioned in argument 1
   cd $filesdir
   # Count the number of files in the directory
   numfiles=$(grep -r $searchstr * | wc -l)
   # Count the number of times the string has been mentioned
   numlines=$(grep -r -o -h $searchstr * | wc -l)
   echo "The number of files are $numfiles and the number of matching lines are $numlines"
   exit 0
else
echo "Error: Invalid file or directory."
exit 1
fi
