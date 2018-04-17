GOOGLEAPIS_GENS_PATH ?= /opt/googleapis/gens

CXX = /opt/webrtc/src/third_party/llvm-build/Release+Asserts/bin/clang++
CPPFLAGS += -MMD -MF -pthread -fPIC -I/usr/local/include -I$(GOOGLEAPIS_GENS_PATH) -m64 -march=x86-64 -O0 -g2 --sysroot=/opt/webrtc/src/build/linux/debian_jessie_amd64-sysroot 
CXXFLAGS += -std=c++11

.PHONY: all
all: googleapis.a
DEPS:= $(shell find $(GOOGLEAPIS_GENS_PATH)/google -name 'protobuf' -prune -o -name '*.pb.cc' -type f -print)

%.o:  %.pb.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $@ -o $?

googleapis.a: $(DEPS:.cc=.o)
	/opt/webrtc/src/third_party/llvm-build/Release+Asserts/bin/llvm-ar rc googleapis.a $?


