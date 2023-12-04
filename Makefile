GPPPARAMS = -Isrc -std=c++11 -Wall -g -lpthread -ldl -lyaml-cpp


OBJECTS = build/Lunar/obj/base/log.cc.o \
    build/Lunar/obj/base/util.cc.o \
    build/Lunar/obj/base/config.cc.o \
    build/Lunar/obj/base/thread.cc.o \
    build/Lunar/obj/base/mutex.cc.o \
    build/Lunar/obj/base/fiber.cc.o\
    build/Lunar/obj/base/scheduler.cc.o\
    build/Lunar/obj/base/iomanager.cc.o\
    build/Lunar/obj/base/timer.cc.o\
    build/Lunar/obj/base/fdmanager.cc.o\
    build/Lunar/obj/base/hook.cc.o\
    build/Lunar/obj/net/address.cc.o\
    build/Lunar/obj/net/socket.cc.o\
    build/Lunar/obj/net/socket.cc.o\
    build/Lunar/obj/net/stream.cc.o\
    build/Lunar/obj/net/socketstream.cc.o\
    build/Lunar/obj/net/bytearray.cc.o\
    build/Lunar/obj/http/http.cc.o\
    build/Lunar/obj/http/httpparser.cc.o\
    build/Lunar/obj/http/httpsession.cc.o\
    build/Lunar/obj/http/http11_parser.rl.cc.o\
    build/Lunar/obj/http/httpclient_parser.rl.cc.o\
    build/Lunar/obj/net/tcpserver.cc.o\
    build/Lunar/obj/http/httpserver.cc.o\
    build/Lunar/obj/http/servlet.cc.o\
    build/Lunar/obj/init/daemon.cc.o\
    build/Lunar/obj/init/env.cc.o\
    build/Lunar/obj/init/application.cc.o\
    build/Lunar/obj/base/worker.cc.o\
    build/Lunar/obj/init/module.cc.o\
    build/Lunar/obj/init/library.cc.o\
    build/Lunar/obj/base/worker.cc.o

TESTS = build/Lunar/tests/test_application.cc.o
# 先生成.o目录

build/Lunar/tests/%.cc.o: tests/%.cc
	mkdir -p ${@D}
	g++ ${GPPPARAMS} -o $@ -c $<

build/Lunar/obj/%.cc.o: src/%.cc
	mkdir -p ${@D}
	g++ ${GPPPARAMS} -o $@ -c $<

# 再生成bin
bin/test_application.bin: ${OBJECTS} ${TESTS}
	g++ ${GPPPARAMS} -o $@ $<


.PHONY: clean
clean:
	rm -rf build/Lunar