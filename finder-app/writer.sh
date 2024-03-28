#!/bin/bash

FILENAME=''
STR=''

if [ $# -lt 2 ]
then
	echo "use $0 filename string"
	exit 1
else
	FILENAME=$1
	STR=$2
fi

echo ${STR} > ${FILENAME}



