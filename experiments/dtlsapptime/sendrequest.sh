#!/bin/bash

REQUEST=$1

while read -p "Use Ctrl^D to start sending GET:" LINE
do
	echo $LINE;
done

while (( 1 ))
do
	echo $REQUEST
	sleep 1
done
