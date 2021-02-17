cmake_minimum_required (VERSION 2.6)
project (videogeneration)
set (target videogeneration)
set (target_json_mapping get_json_mapping)
set (target_audioselector audioselector)

# LibFindMacros
include(LibFindMacros.cmake)

set (ROOT  ${CMAKE_CURRENT_SOURCE_DIR}/..)
set (GUIDO  ${ROOT}/src)
FILE(GLOB SERVER_CPP_SOURCES ${ROOT}/server/*.cpp)

list(REMOVE_ITEM SERVER_CPP_SOURCES "/app/tools/../server/main.cpp")
FILE(GLOB SERVER_C_SOURCES   ${ROOT}/server/*.c)
FILE(GLOB CAIRO_CPP_SOURCES  ${CMAKE_CURRENT_SOURCE_DIR}/../platforms/linux/src/Cairo*.cpp)
FILE(GLOB LOCAL_CPP_SOURCES  ${CMAKE_CURRENT_SOURCE_DIR}/videogeneration.cpp)
FILE(GLOB LOCAL_CPP_SOURCES_JSON_MAPPING  ${CMAKE_CURRENT_SOURCE_DIR}/get_json_mapping.cpp)

FILE(GLOB AUDIOSELECTOR_CPP_SOURCES  ${CMAKE_CURRENT_SOURCE_DIR}/audioselector.cc)

FILE(GLOB REMOTE_CPP_SOURCES  ${CMAKE_CURRENT_SOURCE_DIR}/../platforms/linux/server/*.cpp)

FILE(GLOB JSON_CPP_SOURCES   ${ROOT}/lang/javascript/src/json/*.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platforms/linux/server/)
include_directories(${ROOT}/server)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platforms/linux/src/)
include_directories(${ROOT}/src/engine/include)
include_directories(${ROOT}/src/engine/abstract)
include_directories(${ROOT}/src/engine/devices)
include_directories(${ROOT}/lang/javascript/src/json)
add_definitions(-DJSON_ONLY)


# cairo
libfind_pkg_check_modules(cairo_PKGCONF cairo)
find_path(cairo_INCLUDE_DIR
  NAMES cairo.h
  PATHS ${cairo_PKGCONF_INCLUDE_DIRS}
)
find_library(cairo_LIBRARY
  NAMES cairo
  PATHS ${cairo_PKGCONF_LIBRARY_DIRS}
)
set(cairo_PROCESS_INCLUDES cairo_INCLUDE_DIR)
set(cairo_PROCESS_LIBS cairo_LIBRARY)
libfind_process(cairo)
if(cairo_INCLUDE_DIRS)
  add_definitions("-I" ${cairo_INCLUDE_DIRS})
endif()
set(LINK ${LINK} ${cairo_LIBRARIES})

# libmusicxml
find_path(musicxml_INCLUDE_DIR
  NAMES libmusicxml.h
)
find_library(musicxml_LIBRARY
  NAMES libmusicxml
)
set(musicxml_PROCESS_INCLUDES musicxml_INCLUDE_DIR)
set(musicxml_PROCESS_LIBS musicxml_LIBRARY)
libfind_process(libmusicxml)
if(musicxml_INCLUDE_DIRS)
  add_definitions("-I" ${musicxml_INCLUDE_DIRS})
endif()
message(STATUS "Looking for someheader.h")
message(STATUS ${musicxml_LIBRARIES})
set(LINK ${LINK} ${musicxml_LIBRARIES} -llibmusicxml)
message(STATUS ${LINK})
# microhttpd
libfind_pkg_check_modules(microhttpd_PKGCONF libmicrohttpd)
find_path(microhttpd_INCLUDE_DIR
  NAMES microhttpd.h
  PATHS ${microhttpd_PKGCONF_INCLUDE_DIRS}
)
find_library(microhttpd_LIBRARY
  NAMES microhttpd
  PATHS ${microhttpd_PKGCONF_LIBRARY_DIRS}
)
set(microhttpd_PROCESS_INCLUDES microhttpd_INCLUDE_DIR)
set(microhttpd_PROCESS_LIBS microhttpd_LIBRARY)
libfind_process(microhttpd)
if(microhttpd_INCLUDE_DIRS)
  add_definitions("-I" ${microhttpd_INCLUDE_DIRS})
endif()
set(LINK ${LINK} ${microhttpd_LIBRARIES})

# fluidsynth
libfind_pkg_check_modules(fluidsynth_PKGCONF libfluidsynth)
find_path(fluidsynth_INCLUDE_DIR
  NAMES fluidsynth.h
  PATHS ${fluidsynth_PKGCONF_INCLUDE_DIRS}
)
find_library(fluidsynth_LIBRARY
  NAMES fluidsynth
  PATHS ${fluidsynth_PKGCONF_LIBRARY_DIRS}
)
set(fluidsynth_PROCESS_INCLUDES fluidsynth_INCLUDE_DIR)
set(fluidsynth_PROCESS_LIBS fluidsynth_LIBRARY)
libfind_process(fluidsynth)
if(fluidsynth_INCLUDE_DIRS)
  add_definitions("-I" ${fluidsynth_INCLUDE_DIRS})
endif()
set(LINK ${LINK} ${fluidsynth_LIBRARIES})

# crypto
libfind_pkg_check_modules(crypto_PKGCONF libcrypto)
find_path(crypto_INCLUDE_DIR
  NAMES openssl/sha.h
  PATHS ${crypto_PKGCONF_INCLUDE_DIRS}
)
find_library(crypto_LIBRARY
  NAMES crypto
  PATHS ${crypto_PKGCONF_LIBRARY_DIRS}
)
set(crypto_PROCESS_INCLUDES crypto_INCLUDE_DIR)
set(crypto_PROCESS_LIBS crypto_LIBRARY)
libfind_process(crypto)
if(crypto_INCLUDE_DIRS)
  add_definitions("-I" ${crypto_INCLUDE_DIRS})
endif()
set(LINK ${LINK} ${crypto_LIBRARIES})

# curl
libfind_pkg_check_modules(curl_PKGCONF libcurl)
find_path(curl_INCLUDE_DIR
  NAMES curl/curl.h
  PATHS ${curl_PKGCONF_INCLUDE_DIRS}
)
find_library(curl_LIBRARY
  NAMES curl
  PATHS ${curl_PKGCONF_LIBRARY_DIRS}
)
set(curl_PROCESS_INCLUDES curl_INCLUDE_DIR)
set(curl_PROCESS_LIBS curl_LIBRARY)
libfind_process(curl)
if(curl_INCLUDE_DIRS)
  add_definitions("-I" ${curl_INCLUDE_DIRS})
endif()
set(LINK ${LINK} ${curl_LIBRARIES})

# Magick++
libfind_pkg_check_modules(Magick++_PKGCONF ImageMagick++)
add_definitions(${Magick++_PKGCONF_CFLAGS})
set(LINK ${LINK} ${Magick++_PKGCONF_LDFLAGS})

if(APPLE)
  set(LINK ${LINK}  "-framework GUIDOEngine  -F../../build/MacOS/Release")
  #set(LINK ${LINK}  "-l SGUIDOEngine  -L../../build/MacOS/Release")
else()
  set(LINK ${LINK}  "-l GUIDOEngine")
  LINK_DIRECTORIES(${ROOT}/build/linux/)
endif(APPLE)
set(CMAKE_CXX_FLAGS "-O3 -fpermissive -std=c++11")
# set(CMAKE_CXX_FLAGS "-g -ggdb -std=c++11 -fpermissive")

add_executable(${target} ${LOCAL_CPP_SOURCES} ${REMOTE_CPP_SOURCES} ${SERVER_CPP_SOURCES} ${SERVER_C_SOURCES} ${CAIRO_CPP_SOURCES} ${JSON_CPP_SOURCES})
add_executable(${target_json_mapping} ${LOCAL_CPP_SOURCES_JSON_MAPPING} ${REMOTE_CPP_SOURCES} ${SERVER_CPP_SOURCES} ${SERVER_C_SOURCES} ${CAIRO_CPP_SOURCES} ${JSON_CPP_SOURCES})

add_executable(${target_audioselector} ${AUDIOSELECTOR_CPP_SOURCES})

target_link_libraries(${target} ${LINK})
target_link_libraries(${target_json_mapping} ${LINK})
