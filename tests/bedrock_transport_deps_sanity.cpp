#include <google/protobuf/message.h>
#include <grpcpp/grpcpp.h>

// Conditionally include libsodium if available
#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

#include <cstdlib>

int main() {
    // Simple presence checks; no heavy logic
    
    // Protobuf and gRPC headers are included above - if we got here, they're present
    
    // libsodium initialization (only if available)
#ifdef HAVE_LIBSODIUM
    if (sodium_init() < 0) {
        return 1;
    }
#endif

    return 0;
}

