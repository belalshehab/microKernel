# ── libsodium ─────────────────────────────────────────────────────────────────
find_library(SODIUM_LIB sodium HINTS /opt/homebrew/lib REQUIRED)
find_path(SODIUM_INCLUDE sodium.h HINTS /opt/homebrew/include REQUIRED)

# ── NetworkListener executable ────────────────────────────────────────────────
add_executable(networkListener
    main.cpp
    NetworkListener.cpp
    NetworkListener.h
)

target_include_directories(networkListener PRIVATE ${SODIUM_INCLUDE})
target_link_libraries(networkListener PRIVATE proto ${SODIUM_LIB})

