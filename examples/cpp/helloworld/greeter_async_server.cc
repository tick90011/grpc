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

#include <memory>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "helloworld.grpc.pb.h"

using namespace std;

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

class ServerImpl final
{
 public:
  
   ~ServerImpl() 
  {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run() 
  {
    std::string server_address("0.0.0.0:50051");

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    HandleRpcs();
  }

 private:
  
   // Class encompasing the state and logic needed to serve a request.
  class CallData 
  {
   public:
	   enum ServiceType {
		   ST_HELLO_FUNC1 = 0,
		   ST_HELLO_FUNC2 = 1,
		   ST_HELLO2,
		   ST_HELLO3,
		   ST_HELLO4
	   };
    // Take in the "service" instance (in this case representing an asynchronous
    // server) and the completion queue "cq" used for asynchronous communication
    // with the gRPC runtime.
    CallData(Greeter::AsyncService* service, ServerCompletionQueue* cq, ServiceType type)
        : service_(service), cq_(cq), responder1_(&ctx_), responder2_(&ctx_), status_(CREATE), type_(type)
    {
      std::cout << "new CallData: " << this <<  "type : " << type_ << std::endl;
      // Invoke the serving logic right away.
      Proceed();
    }

    void Proceed() 
    {
      if (status_ == CREATE) 
      {
        // Make this instance progress to the PROCESS state.
        status_ = PROCESS;

		switch (type_)
		{
		case ServerImpl::CallData::ST_HELLO_FUNC1:
			// As part of the initial CREATE state, we *request* that the system
			// start processing SayHello requests. In this request, "this" acts are
			// the tag uniquely identifying the request (so that different CallData
			// instances can serve different requests concurrently), in this case
			// the memory address of this CallData instance.
			service_->RequestSayHelloFunc1(&ctx_, &request1_, &responder1_, cq_, cq_, this);
			break;
		case ServerImpl::CallData::ST_HELLO_FUNC2:
			service_->RequestSayHelloFunc2(&ctx_, &request2_, &responder2_, cq_, cq_, this);
			break;
		default:
			break;
		}

      }
      else if (status_ == PROCESS) 
      {
        // Spawn a new CallData instance to serve new clients while we process
        // the one for this CallData. The instance will deallocate itself as
        // part of its FINISH state.
		  status_ = FINISH;
        new CallData(service_, cq_,type_);

		switch (type_)
		{
		case ServerImpl::CallData::ST_HELLO_FUNC1:

			reply1_.set_message("ST_HELLO_FUNC1 this is from server reply");
			printf("ST_HELLO_FUNC1 Server get from client message %s\n", request1_.name().c_str());

			// And we are done! Let the gRPC runtime know we've finished, using the
			// memory address of this instance as the uniquely identifying tag for
			// the event.
			status_ = FINISH;
			responder1_.Finish(reply1_, Status::OK, this);

			break;
		case ServerImpl::CallData::ST_HELLO_FUNC2:

			reply2_.set_message("ST_HELLO_FUNC2 this is from server reply");
			printf("ST_HELLO_FUNC2 Server get from client message %s\n", request2_.name().c_str());
			status_ = FINISH;
			responder2_.Finish(reply2_, Status::OK, this);
			break;

		case ServerImpl::CallData::ST_HELLO2:
			break;
		case ServerImpl::CallData::ST_HELLO3:
			break;
		case ServerImpl::CallData::ST_HELLO4:
			break;
		default:
			break;
		}
      } else {
        GPR_ASSERT(status_ == FINISH);
        // Once in the FINISH state, deallocate ourselves (CallData).
        delete this;
      }
    }

	ServiceType GetType() const { return type_; }

   private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    Greeter::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;

    // What we get from the client.
    HelloRequest request1_;
    // What we send back to the client.
    HelloReply reply1_;

    // The means to get back to the client.
    ServerAsyncResponseWriter<HelloReply> responder1_;

	// What we get from the client.
	HelloRequest request2_;
	// What we send back to the client.
	HelloReply reply2_;

	// The means to get back to the client.
	ServerAsyncResponseWriter<HelloReply> responder2_;


    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.


	ServiceType type_;
  };


  // This can be run in multiple threads if needed.
  void HandleRpcs() 
  {
    // Spawn a new CallData instance to serve new clients.
	new CallData(&service_, cq_.get(), CallData::ST_HELLO_FUNC1);
	new CallData(&service_, cq_.get(), CallData::ST_HELLO_FUNC2);

    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) 
    {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallData instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);

	  CallData *pCallData = static_cast<CallData*>(tag);

      std::cout << "get new CallData: " << pCallData  << " type: " << pCallData->GetType()<< endl;

	  pCallData->Proceed();
    }
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  Greeter::AsyncService service_;
  std::unique_ptr<Server> server_;
};

int main(int argc, char** argv) {
  ServerImpl server;
  server.Run();

  return 0;
}
