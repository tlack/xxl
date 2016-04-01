BUILDH="yes"
CC="gcc"
NODE="node" # often 'nodejs'
ARCH="-m64"
LIBS="-pthread -ldl "
DEBUG="-pg"
DEBUG="-DDEBUG -g -pg -ggdb3"  # comment out for silence
OPT=""
# OPT="-O3"
DEFS="-DTHREAD $DEBUG $OPT"
WARN="-Wall -Wno-format-extra-args -Wno-unused-function -Wno-unused-value -Wno-char-subscripts"
WARN="$WARN -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format"

# decide what goes into stdlib
STDLIB="-DSTDLIBFILE -DSTDLIBMBOX -DSTDLIBNET -DSTDLIBSHAREDLIB -DSTDLIBSHELL -DSTDLIBXD "

# command to use to run it - put testing args to binary for execution here
RUN="./xxl $*"

errcho() { echo "$@" 1>&2; }

if [ -x /usr/bin/clang ]; then
	errcho using clang
	CC="/usr/bin/clang"
	ARCH=""
	DEFS="$DEFS -fsanitize=address -fno-omit-frame-pointer -DTHREAD_NO_TIMEDLOCK "
	# add Mac-specific definitions here
fi

if [ -x /bin/x86_64-w64-mingw32-gcc-4.9.2.exe ]; then
	errcho using mingw64
	CC="/bin/x86_64-w64-mingw32-gcc-4.9.2.exe"
fi

if [ -x /bin/x86_64-pc-cygwin-gcc.exe ]; then
	errcho using cygwin
	CC="/bin/x86_64-pc-cygwin-gcc.exe"
	DEFS="$DEFS -DTHREAD_NO_TIMEDLOCK"
fi

if [ -f /etc/os-release ]; then
	if grep "Ubuntu" /etc/os-release >/dev/null; then
		errcho using ubuntu -Wl,--no-as-needed 
		LIBS="-Wl,--no-as-needed $LIBS"
	fi
fi

if which rlwrap >/dev/null; then
	errcho using rlwrap
	RUN="rlwrap $RUN"
fi

SRC=`pwd`
COMPILE="$CC $DEFS $WARN $LIBS $ARCH $STDLIB "
COMPILEOBJ="$COMPILE -c "
COMPILESHARED="$COMPILE -fPIC -shared "
BUILDOBJ="$COMPILE -o  "
BUILDSHARED="$COMPILE -fPIC -shared -o "

if [ "x$BUILDH" = "xyes" ]; then
	if hash $NODE 2>/dev/null; then
		$NODE accessors.js > accessors.h && \
		$NODE vary.js > vary.h && \
		$NODE cast.js > cast.h && \
		$NODE types.js > types.h && \
		$NODE repr.js > repr.h 
	else
		errcho "can't find node; skipping build"
	fi
fi

echo "" > compile.h
echo "#define XXL_SRC \"$SRC/\"" >> compile.h
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
$COMPILEOBJ \
	stdlib.c 2>&1 \
	&& \
$BUILDOBJ xxl \
	xxl.o repl.o net.o stdlib.o 2>&1 \
	&& \
$RUN



