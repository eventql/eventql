project(STX_RPC)

include_directories(../)

add_library(stx-rpc STATIC
    RPC.cc
    RPCClient.cc
    ServerGroup.cc)
