BUILDH="yes"
CC="gcc"
NODE="node" # often 'nodejs'
ARCH="-m64"
LIBS="-pthread"
DEBUG=""
DEBUG="-DDEBUG -g"  # comment out for silence
OPT=""
#OPT="-O3"
DEFS="-DTHREAD $DEBUG $OPT"
WARN="-Wall -Wno-format-extra-args -Wno-unused-value -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format"

if [ -x /bin/x86_64-w64-mingw32-gcc-4.9.2.exe ]; then
	echo using mingw64
	CC="/bin/x86_64-w64-mingw32-gcc-4.9.2.exe"
fi

if [ -x /bin/x86_64-pc-cygwin-gcc.exe ]; then
	echo using cygwin
	CC="/bin/x86_64-pc-cygwin-gcc.exe"
fi

if [ "x$BUILDH" = "xyes" ]; then
	$NODE accessors.js > accessors.h && \
	$NODE vary.js > vary.h && \
	$NODE cast.js > cast.h && \
	$NODE types.js > types.h && \
	$NODE repr.js > repr.h 
fi

$CC $DEFS $WARN $LIBS $ARCH \
	xxl.c net.c repl.c -o ./xxl 2>&1 \
	&& ./xxl


