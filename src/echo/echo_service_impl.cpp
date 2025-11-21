#include "echo_service_impl.h"

namespace bedrock::echo {

grpc::Status EchoServiceImpl::Echo(grpc::ServerContext* context,
                                     const EchoRequest* request,
                                     EchoReply* reply) {
    // Simple echo: return the same message
    reply->set_message(request->message());
    return grpc::Status::OK;
}

} // namespace bedrock::echo

