# libFuzzer targets for the untrusted-input parsers. Clang only.
#   cmake -S tests -B build-fuzz -DCMAKE_CXX_COMPILER=clang++ -DHORNET_LINK_FUZZ=ON
#   cmake --build build-fuzz
#   ./build-fuzz/fuzz/fuzz_handshake -max_total_time=60 tests/fuzz/corpus/handshake

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "HORNET_LINK_FUZZ requires Clang (libFuzzer)")
endif()

foreach(target
        fuzz_export_parser
        fuzz_handshake
        fuzz_import_line
        fuzz_rs485_frame
        fuzz_profile_store)
    add_executable(${target} ${target}.cpp)
    target_include_directories(${target} PRIVATE ${HORNET_LINK_SRC_DIR})
    target_compile_options(${target} PRIVATE
        -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all -g)
    target_link_options(${target} PRIVATE -fsanitize=fuzzer,address,undefined)
endforeach()
