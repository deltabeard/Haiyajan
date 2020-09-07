#!/bin/bash

# Obtains the latest gamecontrollerdb.txt and converts it to a header file.
# Copyright (c) 2020  Mahyar Koshkouei
# Public Domain

# Check that the required applications for this script are installed.

FILESRC="gamecontrollerdb"
FILEDEF="gcdb_bin"
URL="https://github.com/gabomdq/SDL_GameControllerDB/raw/master/gamecontrollerdb.txt"

echo "Checking that required applications are available..."

NOTFOUND=0
for prog in "wget" "grep" "xxd" "cp" "rm" "zopfli" "stat"
do
	if ! which $prog > /dev/null; then
		echo "X  $prog not found"
		NOTFOUND=1
	else
		echo "   $prog found"
	fi
done

if [ $NOTFOUND -eq 1 ]; then
	echo "Please install the missing dependencies."
	exit 1
fi

rm -f $FILESRC*

echo "Downloading latest gamecontrollerdb.txt"
if ! wget -q $URL; then
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

	if ! xxd -i ${FILEDEF} > ${FILEDEF}_${os}.h; then
		echo "xxd failed"
		exit $?
	fi

	TXTSZ=$(stat --format=%s ${FILESRC}_${os}.txt)
	echo "const unsigned long gcdb_txt_len = $TXTSZ;" >> ${FILEDEF}_${os}.h
	sed -i "1s/.*/const\ unsigned\ char\ gcdb_bin\[\]\ \=\ \{/" ${FILEDEF}_${os}.h
	sed -i "s/unsigned\ int\ /const\ unsigned\ long\ /" ${FILEDEF}_${os}.h
done

cp "${FILEOUT}"*.h ../inc/
