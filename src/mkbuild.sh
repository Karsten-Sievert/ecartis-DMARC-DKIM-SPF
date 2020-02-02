#!/bin/sh
ver=`awk '/^#define CUR_BUILD_VERSION / { print $3 }' inc/build.h`
#echo "ver = $ver"
newver=$[$ver + 1]
#echo "newver = $newver"
sed -e "s/^#define CUR_BUILD_VERSION $ver/#define CUR_BUILD_VERSION $newver/" inc/build.h > inc/build.h.tmp
mv inc/build.h.tmp inc/build.h
