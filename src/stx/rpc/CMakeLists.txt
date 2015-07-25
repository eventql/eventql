project(FNORD_RPC)

include_directories(../)

add_library(stx-rpc OBJECT
    RPC.cc
    RPCClient.cc
    ServerGroup.cc)
