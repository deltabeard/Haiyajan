#!/bin/bash

# Obtains the latest gamecontrollerdb.txt and converts it to a header file.
# Copyright (c) 2020  Mahyar Koshkouei
# Public Domain

# Check that the required applications for this script are installed.

FILESRC="gamecontrollerdb"
FILEDEF="gcdb_bin"
URL="https://github.com/gabomdq/SDL_GameControllerDB/raw/master/gamecontrollerdb.txt"

echo "Checking that required applications are available..."

for prog in "wget" "grep" "xxd" "cp" "rm" "zopfli" "stat"
do
	which $prog > /dev/null
	if (($?)); then
		echo "$prog not found"
		exit 1
	else
		echo "found $prog"
	fi
done

rm -f $FILESRC*

echo "Downloading latest gamecontrollerdb.txt"
wget -q $URL
if (($?)); then
	echo "wget failed"
	exit $?
fi

grep platform\:Linux ${FILESRC}.txt > ${FILESRC}_linux.txt
grep platform\:Windows ${FILESRC}.txt > ${FILESRC}_windows.txt
mv ${FILESRC}.txt ${FILESRC}_all.txt

echo "Compressing and creating headers"
for os in "linux" "windows" "all"
do
	zopfli -i100 --deflate -c ${FILESRC}_${os}.txt > ${FILEDEF}

	xxd -i ${FILEDEF} > ${FILEDEF}_${os}.h
	if (($?)); then
		echo "xxd failed"
		exit $?
	fi

	TXTSZ=$(stat --format=%s ${FILESRC}_${os}.txt)
	echo "const unsigned long gcdb_txt_len = $TXTSZ;" >> ${FILEDEF}_${os}.h
	sed -i "1s/.*/const\ unsigned\ char\ gcdb_bin\[\]\ \=\ \{/" ${FILEDEF}_${os}.h
	sed -i "s/unsigned\ int\ /const\ unsigned\ long\ /" ${FILEDEF}_${os}.h
done

cp ${FILEOUT}*.h ../inc/
