RUN="./xxl "

errcho() { echo "$@" 1>&2; }

if which rlwrap >/dev/null; then
	errcho using rlwrap
	RUN="rlwrap $RUN"
fi
$RUN $*


