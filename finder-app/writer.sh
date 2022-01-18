#!/bin/bash

# Script for Assignment 1 AESD. It accepts two arguments, a path to a file (including filename) (writefile) and a text string that is to be written into the file(writestr). This script creates a file with filepath writefile and adds content writestr to that file. I file exits it will overwrite the file.
#Author: Anshul Somani

# References: https://ryanstutorials.net/bash-scripting-tutorial/bash-variables.php
# https://ryanstutorials.net/bash-scripting-tutorial/bash-if-statements.php
# https://acloudguru.com/blog/engineering/conditions-in-bash-scripting-if-statements
# https://unix.stackexchange.com/questions/6648/number-of-files-containing-a-given-string
# https://devconnected.com/how-to-check-if-file-or-directory-exists-in-bash/
# https://unix.stackexchange.com/questions/305844/how-to-create-a-file-and-parent-directories-in-one-command

arguments=$#
writefile=$1
writestr=$2
# Go to Home directory
cd
# Check if the number of arguments is less than 2
if [ $arguments -lt 2 ]
 then
   echo "Error: Insufficient number of parameters."
   echo "There should be a total of 2 commands. The first command is the path to the file, including filename that is to be created and the second command should be the string to write into the file."
   echo " ./finder.sh <path> <string>"
   exit 1
# Check if the number of arguments is greater than 2k
elif [ $arguments -gt 2 ]
 then
   echo "Error: Too many parameters."
   echo "There should be a total of 2 commands. The first command is the path to the file, including filename that is to be created and the second command should be the string to write into the file."
   echo " ./finder.sh <path> <string>"
   exit 1
# Check if the file/Directory already exists
elif [[ -f "${filesdir}" || -d "${filesdir}" ]]
 then 
   echo "Directory/File already exists"
   exit 1
else
# Creating the directory and file
#mkdir -p "${writefile%/*}" && touch "$writefile"
# Writing the string to the file
echo "$writestr" > "$writefile"
if [ $? -ne 0 ]
 then 
   echo "Error: File/ Directory couldn't be created"
   exit 1
fi
fi
