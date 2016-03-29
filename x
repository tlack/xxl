DIR=`dirname $(readlink -f $0)`
echo $DIR
XXL=$DIR/xxl
RUN="$XXL "

errcho() { echo "$@" 1>&2; }

if which rlwrap >/dev/null; then
	errcho using rlwrap
	RUN="rlwrap $RUN"
fi
errcho $RUN $*
$RUN $*


