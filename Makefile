PBW = build/pebble-fireflies.pbw

configure:
	./waf configure

compile: configure
	./waf build

install: compile
	${LIBPEBBLE_HOME}/p.py --lightblue load $(PBW)

reinstall: compile
	${LIBPEBBLE_HOME}/p.py --lightblue reinstall $(PBW)

numbers:
	rm src/numbers.h
	for f in `find doc/images/numbers/ -type f -name '*.png'`; do python /Users/nmurray/programming/c/pebble/pebble-sdk-release-001/sdk/tools/bitmapgen.py header $f >> src/numbers.h ; done

glyphs:
	./bin/make-glyphs.sh

