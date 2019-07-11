#!/bin/bash

# @function script
# @author Peparaio Giacomo 530822
# opera originale dell'autore


#controlli iniziali
if [[ ("$1" = "-help") || "$#" -ne 2 || ! ($2 =~ ^[0-9]*$) ]]; then
	echo -e "USE:\n$0 configuation_file mins\n"
	exit -1
fi

DirName=""
while IFS='= ' read -r lhs rhs     #leggo righe fino a ricavarmi il valore di "DirName"
do
if [[ ! $lhs =~ ^\ *# && -n $lhs ]]; then
    rhs="${rhs%%\#*}"
    rhs="${rhs%%*( )}"
    rhs="${rhs%\"*}"
    rhs="${rhs#\"*}"
		if [[ "$lhs" = "DirName" ]]; then
			DirName="$rhs/"
			break
		fi
fi
done < $1
if [[ $DirName = "" ]]; then
	echo -e "DirName di $1 non esistente\n"
	exit -1
fi


IFS=$'\n'
Files=`ls $DirName`
echo -e "$DirName\n"
if [[ $2 = 0 ]]; then  #se è 0 stampo i file
	for f in $Files
	do
		echo $DirName$f
	done
else                   #se è 1 cancello
	for f in $Files
	do
		ts1=$(stat -c %Y "$DirName$f")
		now=$(date '+%s')
		ts2=$(($now - $2*60))
		if [ $ts1 -lt $ts2 ]; then
	     rm $DirName$f
		fi
	done
fi
