# ── libsodium ─────────────────────────────────────────────────────────────────
find_library(SODIUM_LIB sodium HINTS /opt/homebrew/lib REQUIRED)
find_path(SODIUM_INCLUDE sodium.h HINTS /opt/homebrew/include REQUIRED)

# ── KeyGuard executable ────────────────────────────────────────────────────────
add_executable(keyGuard
    main.cpp
    key_guard.cpp
    key_guard.h
)

target_include_directories(keyGuard PRIVATE ${SODIUM_INCLUDE})
target_link_libraries(keyGuard PRIVATE proto ${SODIUM_LIB})
