#pragma once

#include "bedrock_echo.grpc.pb.h"
#include <grpcpp/grpcpp.h>

namespace bedrock::echo {

class EchoServiceImpl final : public EchoService::Service {
public:
    grpc::Status Echo(grpc::ServerContext* context,
                      const EchoRequest* request,
                      EchoReply* reply) override;
};

} // namespace bedrock::echo

