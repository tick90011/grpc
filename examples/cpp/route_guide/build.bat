@echo on
protoc -I=. --grpc_out=. --plugin=protoc-gen-grpc=.\grpc_cpp_plugin.exe route_guide.proto
protoc.exe -I=. --cpp_out=. route_guide.proto
pause