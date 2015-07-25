# cortex-flow.pc.cmake

Name: cortex-flow
Description: Flow Configuration Language Framework
Version: @CORTEX_FLOW_VERSION@
Requires: cortex-base >= 0.10.0
# Conflicts: 
Libs: -L@CMAKE_INSTALL_PREFIX@/lib -lcortex-flow
Cflags: -I@CMAKE_INSTALL_PREFIX@/include
