export PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):~/.local/lib/pkgconfig/

PROTOC = protoc
PROTO_FILE_PATH = ./
GRPC_CPP_PLUGIN = `which grpc_cpp_plugin`
CXX = g++
CXXFLAGS += -std=c++11
CPPFLAGS += `pkg-config --cflags protobuf grpc`
LDFLAGS += `pkg-config --libs protobuf grpc++`\
		   -pthread -Wl,--no-as-needed\
		   -lgrpc++_reflection -Wl,--as-needed\
		   -ldl

all: information.pb.cc information.grpc.pb.cc server client

%.pb.cc: %.proto
	$(PROTOC) -I $(PROTO_FILE_PATH) --cpp_out=. $<

%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTO_FILE_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $<

server: information.pb.o information.grpc.pb.o server.o
	$(CXX) $^ $(LDFLAGS) -o $@

client: information.pb.o information.grpc.pb.o client.o
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.pb.* *.o client server
