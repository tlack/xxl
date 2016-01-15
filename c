nodejs accessors.js > accessors.h && \
nodejs vary.js > vary.h && \
nodejs cast.js > cast.h && \
nodejs types.js > types.h && \
nodejs repr.js > repr.h && \
gcc -DTHREAD -O3 \
	-Wall -Wno-format-extra-args -Wno-unused-value -Wno-unused-variable -Wno-unused-but-set-variable \
	-pthread \
	0.c -o ./0 2>&1 \
	&& ./0


