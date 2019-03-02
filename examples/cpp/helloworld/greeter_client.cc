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

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using namespace std;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

//单独发送一个包, 接收一个包
  std::string SayHello(const std::string& user)
  {
	  // Data we are sending to the server.
	  HelloRequest request;
	  request.set_name(user);

	  // Container for the data we expect from the server.
	  HelloReply reply;

	  // Context for the client. It could be used to convey extra information to
	  // the server and/or tweak certain RPC behaviors.
	  ClientContext context;

	  // The actual RPC.
	  Status status = stub_->SayHello(&context, request, &reply);

	  printf("%s\n", reply.DebugString().c_str());
	  // Act upon its status.
	  if (status.ok()) {
		  return reply.message();
	  }
	  else {
		  std::cout << status.error_code() << ": " << status.error_message()
			  << std::endl;
		  return "RPC failed";
	  }
  }

  //发送HelloRequest 接收到HelloRelpy 数组
  std::list<HelloReply> SayHello2(const std::string& user)
  {
	  // Data we are sending to the server.
	  HelloRequest request;
	  request.set_name(user);

	  // Context for the client. It could be used to convey extra information to
	  // the server and/or tweak certain RPC behaviors.
	  ClientContext context;

	  std::unique_ptr< ::grpc::ClientReader< ::helloworld::HelloReply>> pReader = stub_->SayHello2(&context, request);
	  if (pReader.get() == NULL) {
		  std::cout << "SayHello2 failed " << endl;
		  return   std::list<HelloReply>();
	  }

	  std::list<HelloReply> listHr;
	  HelloReply hr;
	  while (pReader->Read(&hr)) {
		  listHr.push_back(hr);
	  }
	  pReader->Finish();

	  return std::move(listHr);
  }

  //发送一个数组, 接收一个包
  std::string SayHello3(const std::string &user)
  {
	  // Data we are sending to the server.
	  HelloRequest request;
	  request.set_name(user);

	  // Context for the client. It could be used to convey extra information to
	  // the server and/or tweak certain RPC behaviors.
	  ClientContext context;

	  HelloReply reply;
	  std::unique_ptr< ::grpc::ClientWriter< ::helloworld::HelloRequest>> pRet = stub_->SayHello3(&context, &reply);
	  if (!pRet) {
		  return "";
	  }

	  for (int i = 0; i < 10; ++i)
	  {
		  pRet->Write(request);
	  }

	  pRet->WritesDone();
	  cout << reply.message() << endl;

	  cout << "Client SayHello3 Finish Begin" << endl;
	  Status status = pRet->Finish();
	  cout << "Client SayHello3 Finish End" << endl;
	  if (status.ok()) {
		  cout << "ok" << endl;
	  }
	  else {
		  std::cout << status.error_code() << ": " << status.error_message()
			  << std::endl;
	  }
	  
	  return reply.message();
  }

  //发送一个数组, 接收一个包
  std::string SayHello4(const std::string &user)
  {
	  // Data we are sending to the server.
	  HelloRequest request;
	  request.set_name(user);

	  // Context for the client. It could be used to convey extra information to
	  // the server and/or tweak certain RPC behaviors.
	  ClientContext context;
	  std::unique_ptr< ::grpc::ClientReaderWriter< ::helloworld::HelloRequest, ::helloworld::HelloReply>> stream = stub_->SayHello4(&context);
	  for (int i = 0; i < 10000; ++i)
	  {
		  stream->Write(request);
	  }

	  stream->WritesDone();
	  
	  HelloReply hr;
	  while (stream->Read(&hr)) {
		  cout << "SayHello4 Recv: " << hr.message() << endl;
	  }

	  stream->Finish();

	  return "";
  }


 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  GreeterClient greeter(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
  std::string user("world");
/*
  for (int i = 0; i < 10; ++i) {

  }*/

  std::string reply = greeter.SayHello(user);
  std::cout << "SayHello received: " << reply << std::endl;

  auto listReply =  greeter.SayHello2(user);
  cout << "SayHello2 received:  size :  " << listReply.size() << endl;


  reply = greeter.SayHello3(user);
  std::cout << "SayHello3 received: " << reply << std::endl;
  reply = greeter.SayHello3(user);


  greeter.SayHello4(user);

  getchar();
  getchar();
  return 0;
}
