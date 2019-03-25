#!/bin/bash


if [ $# -lt 1 ]
then
	echo Usage: $0 path-to-pod-root 
	exit
else
	echo Checking if folder $1 actually exists...

	if [ ! -e "$1" ]; then
		echo Pod path $1 doesnot exist! Exiting... .
		exit
	fi
fi

podsrcpath="$1/GuidoKit/Classes/Libs/guidolib"

##### Update src
localsrcpath="$(pwd)/../src"
rsync -avz --exclude 'midisharelight' --exclude 'midi2guido' --exclude 'musedata2guido' --exclude 'sibelius2guido' --exclude 'utilities' --exclude 'samples' --exclude '.DS_Store' $localsrcpath $podsrcpath
localplatformpath="$(pwd)/../platforms"
rsync -avz --exclude 'linux' --exclude 'Max' --exclude 'PureData' --exclude 'win32' --exclude 'android' --exclude '.DS_Store' --exclude 'obsolete' --exclude 'GuidoFontMetrics' --exclude 'GuidoQuartzFontViewer' $localplatformpath $podsrcpath