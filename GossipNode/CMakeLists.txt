# ── libsodium ─────────────────────────────────────────────────────────────────
find_library(SODIUM_LIB sodium HINTS /opt/homebrew/lib REQUIRED)
find_path(SODIUM_INCLUDE sodium.h HINTS /opt/homebrew/include REQUIRED)


find_program(NIM_EXECUTABLE nim REQUIRED)
set(NIM_SRC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/nim)
set(NIM_SRC_FILE ${NIM_SRC_DIR}/gossip_node.nim)
set(NIM_OUT_DIR  ${CMAKE_BINARY_DIR}/nimcache)
set(NIM_LIB      ${NIM_OUT_DIR}/gossip_node.a)

add_custom_command(
        OUTPUT  ${NIM_LIB}
        COMMAND ${NIM_EXECUTABLE} c
        --noMain
        --app:staticLib
        --outdir:${NIM_OUT_DIR}
        --nimcache:${NIM_OUT_DIR}
        -o:${NIM_LIB}
        ${NIM_SRC_FILE}
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
