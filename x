DIR=`dirname $(readlink -f $0)`
echo $DIR
XXL=$DIR/xxl
RUN="$XXL "

errcho() { echo "$@" 1>&2; }

if which rlwrap >/dev/null; then
	errcho using rlwrap
	RUN="rlwrap $RUN"
fi

if which llvm-symbolizer-3.6 >/dev/null; then
	errcho using ASAN_SYMBOLIZER_PATH
	ASAN_SYMBOLIZER_PATH=`which llvm-symbolizer-3.6`
	export ASAN_SYMBOLIZER_PATH
fi

errcho $RUN $*
$RUN $*


