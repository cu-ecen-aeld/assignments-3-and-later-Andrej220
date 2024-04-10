#!/bin/sh

FILESDIR=''
STR=''

if [ $# -lt 2 ]
then
	echo "use $0 dir search_string"
	exit 1
else
	FILESDIR=$1
	STR=$2
fi

if ! [ -d "${FILESDIR}"  ]
then
	echo "${FILESDIR} does not exist."
	exit 1
fi
NUM_FINDINGS=$(grep -l "$STR" ${FILESDIR}/* 2>/dev/null| wc -l)
TOTAL_FILES=$(ls -p ${FILESDIR}| grep -v / | wc -l)

echo "The number of files are ${TOTAL_FILES} and the number of matching lines are ${NUM_FINDINGS}"

