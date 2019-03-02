/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <windows.h>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

using namespace std;

#define  COUT_THREADID() cout << "threadId: " << GetCurrentThreadId() <<endl

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {

	//单独发送一个包, 接收一个包
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override 
  {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
	printf("Get SayHello %s\n", request->name().c_str());
	COUT_THREADID();

	return Status::OK;
  }


  //发送HelloRequest 接收到HelloRelpy 数组
  ::grpc::Status SayHello2(::grpc::ServerContext* context, 
	  const ::helloworld::HelloRequest* request,
	  ::grpc::ServerWriter< ::helloworld::HelloReply>* writer)
  {
	  std::string prefix("Hello2 ");
	  for (int i = 0; i < 1000; ++i)
	  {
		  HelloReply hello;
		  hello.set_message(prefix + request->name());
		  writer->Write(hello);
	  }
	  printf("Get SayHello2 %s\n", request->name().c_str());
	  COUT_THREADID();
	  return Status::OK;
  }

  //发送一个数组, 接收一个包
  ::grpc::Status SayHello3(::grpc::ServerContext* context, ::grpc::ServerReader< ::helloworld::HelloRequest>* reader, ::helloworld::HelloReply* response)
  {
	  std::string prefix("Hello2 ");

	  HelloRequest req;
	  while (reader->Read(&req))
	  {
		  cout << "SayHello3 Recv: " << req.name() <<endl;
	  }
	  response->set_message(prefix + req.name());
	  COUT_THREADID();
	  return Status::OK;
  }


  //发送一个list ,返回一个list
  ::grpc::Status SayHello4(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::helloworld::HelloReply, ::helloworld::HelloRequest>* stream)
  {
	  std::string prefix("SayHello4 ");

	  HelloRequest req;
	  while (stream->Read(&req))
	  {
		  cout << "SayHello4 Recv: " << req.name() << endl;

		  HelloReply hello;
		  hello.set_message(prefix + req.name());
		  stream->Write(hello);

		  Sleep(1000);
	  }
	  
	  COUT_THREADID();

	  return Status::OK;
  }


};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  GreeterServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
