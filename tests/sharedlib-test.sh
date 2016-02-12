gcc -Wall -pedantic -DSTDLIBSHAREDLIB -fPIC -shared \
	sharedlib-test.c ../xxl.o ../repl.o ../net.o -o sharedlib-test.so
