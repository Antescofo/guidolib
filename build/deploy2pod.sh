#!/bin/bash

# Bash script for deploying built libraries and headers to Pod

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

##### Build XCode project if it doesn't exist
bash buildXCodeProject.sh 

##### Build iphoneos and iphonesimulator
xcodebuild -project ios/guidolib.xcodeproj  -target GUIDOEngine -configuration Release -sdk iphoneos CONFIGURATION_BUILD_DIR=ios/Release-iphoneos clean build 
xcodebuild -project ios/guidolib.xcodeproj  -target GUIDOEngine -configuration Release -arch x86_64 only_active_arch=no -sdk iphonesimulator CONFIGURATION_BUILD_DIR=ios/Release-iphonesimulator clean build 


##### Move to Pod location
libname="/libGUIDOEngine.a"

# pod paths
podlibpath="$1/GuidoKit/Classes/Libs/guidolib$libname"
podsrcpath="$1/GuidoKit/Classes/Libs/guidolib"

##### Make Universal Lib and copy to Pod
# Check if Release-iphoneos and Release-iphonesimulator libraries exists
iphonebuildpath="$(pwd)/ios/Release-iphoneos$libname"
simulatorbuildpath="$(pwd)/ios/Release-iphonesimulator$libname"
universalpath="$(pwd)/ios$libname"
if [ -f "$iphonebuildpath" ]
then
	echo iPhoneOS lib exists!
else
	echo Build the iPhoneOS library in $iphonebuildpath ! Exiting... .
	exit
fi

if [ -f $simulatorbuildpath ]
then
	echo iPhoneSimulator lib exists!
else
	echo Build the simulator library in ./Release-iphonesimulator ! Exiting... .
	exit
fi

# Lipo them together
lipo -create $iphonebuildpath $simulatorbuildpath -output $universalpath

if [ $? -eq 0 ]; then
	echo Lipo built the Universal library! Copying to Pod.
else
	echo Lipo failed! Exiting... .
	exit
fi

cp -v $universalpath $podlibpath
if [ $? -eq 0 ]; then
	echo Universal Library copied to Pod.
else
	echo Copy failed! Exiting... .
	exit
fi

##### Update src
localsrcpath="$(pwd)/../src"
rsync -avz --exclude 'midisharelight' --exclude 'midi2guido' --exclude 'musedata2guido' --exclude 'sibelius2guido' --exclude 'utilities' --exclude 'samples' --exclude '.DS_Store' $localsrcpath $podsrcpath
localplatformpath="$(pwd)/../platforms"
rsync -avz --exclude 'linux' --exclude 'Max' --exclude 'PureData' --exclude 'win32' --exclude 'android' --exclude '.DS_Store' --exclude 'obsolete' --exclude 'GuidoFontMetrics' --exclude 'GuidoQuartzFontViewer' $localplatformpath $podsrcpath

##### Copy TTF font to Pod Asset
podassetpath="$1/GuidoKit/Assets"
guidofontpath="$(pwd)/../src/*.ttf"
cp -v $guidofontpath $podassetpath


