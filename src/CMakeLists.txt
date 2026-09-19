# itch-book library
file(GLOB_RECURSE LIB_SOURCES "parser/*.cpp" "book/*.cpp" "feed/*.cpp" "utils/*.cpp")

# CMake requires at least one source file for a static library. Add dummy.cpp just in case.
add_library(itch-book-lib STATIC ${LIB_SOURCES} dummy.cpp)

target_include_directories(itch-book-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(itch-book-lib PUBLIC
    tsl::robin_map
    ZLIB::ZLIB
)

# itch-book executable
add_executable(itch-book main.cpp)
target_link_libraries(itch-book PRIVATE itch-book-lib)
