#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Script requires arguments filename and previous result"
    echo "Example -  ./script <filename> <previous test result>"
    exit
fi

if [[ $2 =~ [:alpha:] ]]
then
    echo "Previous result in second argument should be numeric"
    exit
fi

if [ -f "$1" ]; then
    echo "Hang on getting result!!!"
else
   echo "Please enter a valid file"
   echo "$1 file does not exist"
    exit
fi

FILE=$1 #get the file name
PREV=$2 #store value of previous result to compare

INDEX=$(awk '{for(i=1;i<=NF;i++) if($i ~ /^Throughput/ ) print i}' $FILE)


AVG=$(sed -n '/Throughput/,$ p' $FILE | awk -v va="$INDEX" '{if ($va ~ /[0-9]+$/) {sum += $va; n++}} END {print sum/n}')


echo "Average of Current Throughput is: $AVG"


if awk -v x=$AVG -v y=$PREV 'BEGIN { exit (x == y) ? 0 : 1 }'
then
  echo PASS
else
  echo FAIL
fi


exit
