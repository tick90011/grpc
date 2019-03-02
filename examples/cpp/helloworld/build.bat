@echo on
protoc -I=. --grpc_out=. --plugin=protoc-gen-grpc=.\grpc_cpp_plugin.exe helloworld.proto
protoc.exe -I=. --cpp_out=. helloworld.proto
pause