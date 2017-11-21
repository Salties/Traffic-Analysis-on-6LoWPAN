#!/bin/bash

HELPMSG="Usage: extract [-h|--help] [-s|--sort] [-a|--all] [-t|--time] [-n|--nonzero] LOGFILE"

if [ $# -eq 0 ]
then
	echo $HELPMSG;
	exit;
fi

for arg in "$@"
do
	case $arg in
		-a|--all)
			ALL=1;
			;;
		-t|--time)
			TIME=1;
			;;
		-n|--nonzero)
			NONZERO=1;
			;;
		-s|--sort)
			SORT=1;
			;;
		-r|--responsed)
			RESPONSED=1;
			;;
		-h|--help)
			echo $HELPMSG;
			exit;
			;;
		*)
			LOGFILE+="$arg ";
			;;
	esac
done

#printf "Logfile: %s\n" $LOGFILE;

cat $LOGFILE | gawk  '
#BEGIN{print "---BEGIN---"}
{
if($1 !~ /rtt|---|PING/ && $2 != "packets" && $3 ~ /bytes|answer/)
	{
		time=$1;
		seq=$6;
		ping=$8;
		if($time!="" && ping=="") ping="time=0";
		#print time"\t"ping
		print seq"\t"time"\t"ping
	}
}
#END{print "---END---"}'|\
sed -r 's/\[//g; s/\]//g; s/icmp_seq=//g; s/time=//g;' | sort -n |\
gawk -v all=$ALL -v time=$TIME -v nonzero=$NONZERO -v sorted=$SORT -v responsed=$RESPONSED '
{
	pingtimes[$1] = $2;
	pingvalues[$1] = $3;
}
END {
	if ( sorted )
	{
		asort(pingvalues);
		for ( i = 1; i in pingvalues; i++){
			if( responsed && pingvalues[i] == 0 )
				continue;
			printf "%-5.1f\n", pingvalues[i];
		}
	}
	else 
	{
		basetime = pingtimes[1];
		for (i = 1; i in pingvalues; i++){
			if( responsed && pingvalues[i] == 0 )
				continue;
			if( all )
				printf "%-5d\t%-10.5f\t%-5.1f\n", i, (pingtimes[i] - basetime), pingvalues[i];	#Print icmp_sqe, date and ping.
			else if( time )
				printf "%-10.5f\t%-5.1f\n", (pingtimes[i] - basetime), pingvalues[i];		#Print date and ping.
			else if( nonzero )
				printf "%d\n", pingvalues[i];							#Print ping only in integers.
			else
				printf "%-5.1f\n", pingvalues[i];						#Print ping only.
		}
	}
}
'
