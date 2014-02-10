#!/bin/bash
# from a run list, prints the list of runs that were taken in a specific atmosphere, summer(22) or winter(21)
# written by Nathan Kelley-Hoskins Sept 2013

CONORM="\e[0m"
CORED='\e[1;31m'

ISPIPEFILE=`readlink /dev/fd/0` # check to see if input is from terminal, or from a pipe
if [[ "$ISPIPEFILE" =~ ^/dev/pts/[0-9]{1,2} ]] ; then # its a terminal (not a pipe)
	if ! [ $# -eq 2 ] ; then # the human didn't add any arguments, and we must tell them so
		echo
		echo "From a runlist or pipe, prints the run numbers that are of a particular atmosphere."
		echo " $ `basename $0` [w|21|s|22] <file of runs>" ; echo
		echo "w = 21 = winter, s = 22 = summer" ; echo
		echo "Print list of summer runs:"
		echo " $ `basename $0` s myrunlist.dat" ; echo
		echo "Print list of winter runs:"
		echo " $ `basename $0` 21 myrunlist.dat" ; echo
		echo "Works with pipes : " 
		echo " $ cat myrunlist.dat | `basename $0` w" ; echo
		echo -e "${CORED}WARNING!${CONORM}"
		echo "   The summer/winter status is crudely calculated as "
		echo "      'summer is any run in the months of May through October, inclusive (months 5,6,7,8,9,10)'."
		echo "      'winter is any run in the months of November through April, inclusive (months 11,12,1,2,3,4)'."
		echo "   Since that is not the exact way Summer is defined (the boundary between summer/winter moves each year), tread carefully when using this script." ; echo
		exit
	fi
fi

function echoerr(){ echo "$@" 1>&2; } #for spitting out error text

# list of run_id's to read in
RUNFILE=$2
if [ ! -e $RUNFILE ] ; then
	echo "File $RUNFILE could not be found in $PWD , sorry."
	exit	
fi
RUNLIST=`cat $RUNFILE`
#echo "RUNLIST:$RUNLIST"

SUMMFLAG=false
WINTFLAG=false
LOWARG=`echo "$1" | tr '[A-Z]' '[a-z]'` # make all uppercase letters in arg 1 lowercase, for easier handling
#echo "\$LOWARG: '$LOWARG'"
if [[ "$LOWARG" == *w* ]] || [[ "$LOWARG" == "21" ]] ; then
	WINTFLAG=true
fi
if [[ "$LOWARG" == *s* ]] || [[ "$LOWARG" == "22" ]] ; then
	SUMMFLAG=true
fi
#echo "\$WINTFLAG:$WINTFLAG    \$SUMMFLAG:$SUMMFLAG"
if $WINTFLAG && $SUMMFLAG ; then
	echo "recognized both summer[s] and winter[w] in arg '$1', can only specify one or the other"
	exit
elif ! $WINTFLAG && ! $SUMMFLAG ; then
	echo "Need to specifiy an atmosphere: Argument 1 '$1' needs to be either 'w' or 's'."
	exit
fi

function IsWinter {
    local date="$1"
    local month=${date:4:2}
    if  [ "$date" -gt "20071026" ] && [ "$date" -lt "20080420" ] ||
        [ "$date" -gt "20081113" ] && [ "$date" -lt "20090509" ] ||
        [ "$date" -gt "20091102" ] && [ "$date" -lt "20100428" ] ||
        [ "$date" -gt "20101023" ] && [ "$date" -lt "20110418" ] ||
        [ "$date" -gt "20111110" ] && [ "$date" -lt "20120506" ] ||
        [ "$date" -gt "20121029" ] && [ "$date" -lt "20130425" ] ; then
		#echo true
		echo 1  # winter
	elif [ "$date" -ge "20130425" -o "$date" -le "20071026" ] ; then
        # don't have specific dates for summer/winter boundary, so we will generalize to the months
        # may through october inclusive is 'summer'
        if   [ "$month" -ge 5 -a "$month" -le 10 ] ; then
            echo 2 # summer
        # november through april inclusive is 'winter'
        elif [ "$month" -le 4 -o "$month" -ge 11 ] ; then
            echo 1 # winter
        else
            echo 3 # unassignable
        fi
    else
		#echo false
		echo 2 # summer
    fi
}

function badAtmosphere {
	echoerr "Error, in 'whichRunsAreAtmosphere.sh', run $2 is too new and could not have its atmosphere assigned on date $1, exiting..."
	exit 1
}

# get database url from parameter file
MYSQLDB=`grep '^\*[ \t]*DBSERVER[ \t]*mysql://' "$VERITAS_EVNDISP_ANA_DIR/ParameterFiles/EVNDISP.global.runparameter" | egrep -o '[[:alpha:]]{1,20}\.[[:alpha:]]{1,20}\.[[:alpha:]]{1,20}'`
    
if [ ! -n "$MYSQLDB" ] ; then
    echo "* DBSERVER param not found in \$VERITAS_EVNDISP_ANA_DIR/ParameterFiles/EVNDISP.global.runparameter!"
    exit
#else
#    echo "MYSQLDB: $MYSQLDB"
fi 

# mysql login info
MYSQL="mysql -u readonly -h $MYSQLDB -A"

# generate list of runs to ask for ( run_id = RUNID[1] OR run_id = RUNID[2] etc)
COUNT=0
SUB=""
for ARUN in $RUNLIST ; do
	if [[ "$COUNT" -eq 0 ]] ; then
		SUB="run_id = $ARUN"
	else 
		SUB="$SUB OR run_id = $ARUN"
	fi
	COUNT=$((COUNT+1))
done
#echo "SUB:"
#echo "$SUB"

# search through mysql result rows, where each row's elements
# are assigned to RUNID and RUNDATE
while read -r RUNID RUNDATE ; do
	if [[ "$RUNID" =~ ^[0-9]+$ ]] ; then
		
		# decode the date tag
		read YY MM DD HH MI SE <<< ${RUNDATE//[-:]/ }
		#echo "  YEARMONTHDAY:$YY$MM$DD"

        STATUSFLAG=$( IsWinter "$YY$MM$DD" )
        
        #if   [ "$STATUSFLAG" == "1" ] ; then
        #    STATUSWORD="winter"
        #elif [ "$STATUSFLAG" == "2" ] ; then
        #    STATUSWORD="summer"
        #elif [ "$STATUSFLAG" == "3" ] ; then
        #    STATUSWORD="unassignable"
        #else
        #    STATUSWORD="errorWithSTATUSFLAG"
        #fi
        #echoerr "run $RUNID  - date $YY$MM$DD - status $STATUSWORD"
		
		# is the run month between May(5) and Oct(10), inclusive?
		if $SUMMFLAG ; then
			if   [ "$STATUSFLAG" -eq "2" ] ; then echo "$RUNID"
			elif [ "$STATUSFLAG" -eq "3" ] ; then badAtmosphere "$YY$MM$DD" "$RUNID"
			fi
		elif $WINTFLAG ; then
			if   [ "$STATUSFLAG" -eq "1" ] ; then echo "$RUNID"
			elif [ "$STATUSFLAG" -eq "3" ] ; then badAtmosphere "$YY$MM$DD" "$RUNID"
			fi
		fi
		
		
		
	fi
# This is where the MYSQL command is executed, with the list of requested runs
# You have to do it this way, because using a pipe | calls the command in a
# subshell, and that prevents variables from being saved within the 'while' loop
# http://stackoverflow.com/questions/14585045/is-it-possible-to-avoid-pipes-when-reading-from-mysql-in-bash
done < <($MYSQL -e "USE VERITAS ; SELECT run_id, data_start_time FROM tblRun_Info WHERE $SUB")

exit

