BUILDH="no"
CC="gcc"
NODE="node" # often 'nodejs'
ARCH="-m64"
LIBS="-pthread "
DEBUG="-pg"
DEBUG="-DDEBUG -g -pg -ggdb3"  # comment out for silence
OPT=""
# OPT="-O3"
DEFS="-DTHREAD $DEBUG $OPT"
WARN="-Wall -Wno-format-extra-args -Wno-unused-function -Wno-unused-value "
WARN="$WARN -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format"

# decide what goes into stdlib
STDLIB="-DSTDLIBFILE -DSTDLIBNET -DSTDLIBSHAREDLIB -DSTDLIBSHELL "

# command to use to run it - put testing args to binary for execution here
RUN="./xxl $*"

if [ -x /usr/bin/clang ]; then
	echo using clang
	CC="/usr/bin/clang"
	ARCH=""
	# add Mac-specific definitions here
fi

if [ -x /bin/x86_64-w64-mingw32-gcc-4.9.2.exe ]; then
	echo using mingw64
	CC="/bin/x86_64-w64-mingw32-gcc-4.9.2.exe"
fi

if [ -x /bin/x86_64-pc-cygwin-gcc.exe ]; then
	echo using cygwin
	CC="/bin/x86_64-pc-cygwin-gcc.exe"
fi

if which rlwrap >/dev/null; then
	echo using rlwrap
	RUN="rlwrap $RUN"
fi

SRCPATH=`pwd`
COMPILE="$CC $DEFS $WARN $LIBS $ARCH $STDLIB "
COMPILEOBJ="$COMPILE -c "
COMPILESHARED="$COMPILE -fPIC -shared "
BUILDOBJ="$COMPILE -o  "
BUILDSHARED="$COMPILE -fPIC -shared -o "

if [ "x$BUILDH" = "xyes" ]; then
	$NODE accessors.js > accessors.h && \
	$NODE vary.js > vary.h && \
	$NODE cast.js > cast.h && \
	$NODE types.js > types.h && \
	$NODE repr.js > repr.h 
fi

echo "" > compile.h
echo "#define XXL_SRCPATH \"$SRCPATH\"" >> compile.h
echo "#define XXL_COMPILEOBJ \"$COMPILEOBJ\"" >> compile.h
echo "#define XXL_COMPILESHARED \"$COMPILESHARED\"" >> compile.h
echo "#define XXL_BUILDOBJ \"$BUILDOBJ\"" >> compile.h
echo "#define XXL_BUILDSHARED \"$COMPILESHARED\"" >> compile.h

$COMPILEOBJ \
	xxl.c 2>&1 \
	&& \
$COMPILEOBJ \
	repl.c 2>&1 \
	&& \
$COMPILEOBJ \
	net.c 2>&1 \
	&& \
$BUILDOBJ xxl \
	xxl.o repl.o net.o 2>&1 \
	&& \
clear && $RUN



