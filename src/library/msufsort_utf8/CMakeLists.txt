
add_library(msufsort_utf8
    ./private/msufsort_utf8.cpp
    ./filter/utf-8.cpp
    msufsort_utf8.cpp
)


target_link_libraries(msufsort_utf8 
    common)


target_include_directories(msufsort_utf8
    PUBLIC
        "${_range_v3_include_dir}"
        "${_type_safe_include_dir}"
        $<BUILD_INTERFACE:${_msufsort_utf8_include_dir}>
        $<INSTALL_INTERFACE:include/msufsort_utf8>
)


target_compile_features(msufsort_utf8 PUBLIC cxx_std_20)
