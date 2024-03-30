#!/bin/bash

FILENAME=''
STR=''
if [ "$#" -lt 2 ]
then
	echo "use $0 filename string"
	exit 1
else
	FILENAME=$1
	STR=$2
fi

if ! [ -d ${FILENAME%/*} ]
then
	mkdir  ${FILENAME%/*} 2>/dev/null
fi

if  [ "$?" -ne 0 ]
then
	echo "Direcory ${FILENAME%/*} could not be created."
	exit 1
fi

touch ${FILENAME} 2>/dev/null

if  [ "$?" -ne 0 ]
then
	echo "File ${FILENAME} could not be created."
	exit 1
fi

echo ${STR} > ${FILENAME}



