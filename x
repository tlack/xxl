DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
XXL=$DIR/xxl
RUN="$XXL "

errcho() { echo "$@" 1>&2; }

if which rlwrap >/dev/null; then
	errcho using rlwrap
	RUN="rlwrap $RUN"
fi
$RUN $*


