#!/bin/bash

# Obtains the latest gamecontrollerdb.txt and converts it to a header file.
# Copyright (c) 2020  Mahyar Koshkouei
# Public Domain

# Check that the required applications for this script are installed.

FILESRC="gamecontrollerdb.txt"
FILEOUT="gamecontrollerdb.h"
URL="https://github.com/gabomdq/SDL_GameControllerDB/raw/master/gamecontrollerdb.txt"

echo "Checking that required applications are available..."

for prog in "wget" "grep" "xxd" "cp" "rm"
do
	which $prog > /dev/null
	if (($?)); then
		echo "$prog not found"
		exit 1
	else
		echo "found $prog"
	fi
done

wget $URL
if (($?)); then
	echo "wget failed"
	exit $?
fi

xxd -i $FILESRC > $FILEOUT
if (($?)); then
	echo "xxd failed"
	exit $?
fi

cp $FILEOUT ../inc/$FILEOUT
rm -f $FILESRC*
