PBW = build/pebble-fireflies.pbw

configure:
	./waf configure

compile: configure
	./waf build

install: compile
	${LIBPEBBLE_HOME}/p.py --lightblue load $(PBW)

reinstall: compile
	${LIBPEBBLE_HOME}/p.py --lightblue reinstall $(PBW)


