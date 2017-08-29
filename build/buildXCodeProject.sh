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