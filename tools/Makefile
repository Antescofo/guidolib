
INC = -I../src/engine/include -I../src/engine/devices -I../platforms -I../server -I../lang/javascript/src/json

system	:= $(shell uname -s)
ifeq ($(system), Darwin)
  lib =  -stdlib=libc++ -mmacosx-version-min=10.7 -F../build/MacOS/Release/ -framework GUIDOEngine
  INC := $(INC)  -I../platforms/macosx/src
else
ifeq ($(system), MINGW32_NT-6.1)
  lib =  ../../build/win32/Release/GUIDOEngine.lib
else
  lib = -L../build/lib -lGUIDOEngine
  INC := $(INC)  -I../platforms/linux/src $(shell pkg-config --cflags cairo)
endif
endif

TOOLS = sizedebug videogeneration

all: $(TOOLS)

install: all
	cp $(TOOLS) /usr/local/bin/

videogeneration: videogeneration.cpp
	mkdir -p build
	cd build; cmake ..; make
	cp build/videogeneration .

# videogeneration: videogeneration.cpp
# 	$(CXX) -DJSON_ONLY $(videogensrc) videogeneration.cpp $(INC) -I/usr/include/ImageMagick-6 -I/usr/include/x86_64-linux-gnu/ImageMagick-6/ $(lib) -lcairo -L/usr/lib/x86_64-linux-gnu -lMagick++-6.Q16 -lmusicxml2 -o $@

sizedebug : sizedebug.cpp
	$(CXX) sizedebug.cpp $(INC) $(lib) -o sizedebug


clean:
	rm -f $(TOOLS)
