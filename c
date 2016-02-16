BUILDH="yes"
CC="gcc"
NODE="node" # often 'nodejs'
ARCH="-m64"
LIBS="-pthread "
DEBUG=""
DEBUG="-DDEBUG -g"  # comment out for silence
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

if [ "x$BUILDH" = "xyes" ]; then
	$NODE accessors.js > accessors.h && \
	$NODE vary.js > vary.h && \
	$NODE cast.js > cast.h && \
	$NODE types.js > types.h && \
	$NODE repr.js > repr.h 
fi

echo "#define XXL_COMPILE \"$CC $DEFS $WARN $LIBS $ARCH $STDLIB\"" > compile.h

$CC $DEFS $WARN $LIBS $ARCH $STDLIB \
	xxl.c -c 2>&1 \
	&& \
$CC $DEFS $WARN $LIBS $ARCH $STDLIB \
	repl.c -c 2>&1 \
	&& \
$CC $DEFS $WARN $LIBS $ARCH $STDLIB \
	net.c -c 2>&1 \
	&& \
$CC $DEFS $WARN $LIBS $ARCH $STDLIB \
	xxl.o repl.o net.o -o ./xxl 2>&1 \
	&& \
$RUN



