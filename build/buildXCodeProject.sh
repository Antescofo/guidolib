#!/bin/sh

if [ ! -d ./ios/guidolib.xcodeproj  ]; then
   if [ ! -d ios  ]; then
	mkdir ios
   fi
	cd ./ios
	cmake .. -DMIDIEXPORT=no -DIOS=yes -G Xcode
	cd ..
else
  echo "./guidolib/build/ios/guidolib.xcodeproj already  exists"
fi

if [ ! -d ./macos/guidolib.xcodeproj  ]; then
if [ ! -d macos  ]; then
mkdir macos
fi
cd ./macos
cmake .. -DMIDIEXPORT=no -DIOS=no -G Xcode
cd ..
else
echo "./guidolib/build/ios/guidolib.xcodeproj already  exists"
fi
