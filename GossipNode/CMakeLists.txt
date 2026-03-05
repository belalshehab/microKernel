# ── libsodium ─────────────────────────────────────────────────────────────────
find_library(SODIUM_LIB sodium HINTS /opt/homebrew/lib REQUIRED)
find_path(SODIUM_INCLUDE sodium.h HINTS /opt/homebrew/include REQUIRED)

find_program(NIM_EXECUTABLE nim HINTS /opt/homebrew/bin REQUIRED)

set(NIM_SRC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/nim)
set(NIM_SRC_FILE ${NIM_SRC_DIR}/gossip_node.nim)
set(NIM_OUT_DIR  ${CMAKE_BINARY_DIR}/nimcache)
set(NIM_LIB      ${NIM_OUT_DIR}/gossip_node.a)

# Build a shell one-liner that discovers all --path: flags from ~/.nimble/pkgs2
# then invokes nim directly (no nimble exec needed)
set(NIM_BUILD_SCRIPT "${CMAKE_BINARY_DIR}/nim_gossip_build.sh")
file(WRITE ${NIM_BUILD_SCRIPT}
"#!/bin/sh
NIM_PKGS=\"$HOME/.nimble/pkgs2\"
PATHS=\"\"
if [ -d \"$NIM_PKGS\" ]; then
    for d in \"$NIM_PKGS\"/*/; do
        [ -d \"$d\" ] && PATHS=\"$PATHS --path:$d\"
    done
fi
exec ${NIM_EXECUTABLE} c \
    --noMain \
    --app:staticLib \
    --outdir:${NIM_OUT_DIR} \
    --nimcache:${NIM_OUT_DIR} \
    -o:${NIM_LIB} \
    $PATHS \
    ${NIM_SRC_FILE}
")
file(CHMOD ${NIM_BUILD_SCRIPT} PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)

add_custom_command(
        OUTPUT  ${NIM_LIB}
        COMMAND ${NIM_BUILD_SCRIPT}
        WORKING_DIRECTORY ${NIM_SRC_DIR}
        DEPENDS ${NIM_SRC_FILE}
        COMMENT "Compiling Nim GossipNode..."
)

add_custom_target(nim_gossip_node_lib DEPENDS ${NIM_LIB})


# ── GossipNode executable ─────────────────────────────────────────────────────
add_executable(gossipNode
        main.cpp
        GossipNode.cpp
        GossipNode.h
        nim_gossip_node.h
)

add_dependencies(gossipNode nim_gossip_node_lib)

target_include_directories(gossipNode PRIVATE
        ${SODIUM_INCLUDE}
        ${NIM_OUT_DIR}
)

target_link_libraries(gossipNode PRIVATE
        proto
        ${SODIUM_LIB}
        ${NIM_LIB}
        pthread
)
