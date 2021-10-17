
find_library(LIBCXX_LIB c++)
find_package(Threads)
find_library(LIBCXXABI_LIB c++abi)

link_libraries(
    ${LIBCXX_LIB}
    ${LIBCXXABI_LIB}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(msufsort_utf8_demo main.cpp)

target_link_libraries(msufsort_utf8_demo 
    fmt::fmt
    ${CMAKE_THREAD_LIBS_INIT}
    common 
    msufsort_utf8)