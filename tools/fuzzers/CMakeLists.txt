# Copyright (c) Facebook, Inc. and its affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

if(HERMES_ENABLE_FUZZING)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(HERMES_ENABLE_EH ON)

    add_hermes_tool(fuzzer-jsi-entry
      fuzzer-json.cpp
            json.pb.cc
            jsonConv.cpp
      ${ALL_HEADER_FILES}
    )

    target_compile_options(fuzzer-jsi-entry PRIVATE "-fsanitize=fuzzer-no-link")
    append(${HERMES_FUZZING_FLAG} CMAKE_EXE_LINKER_FLAGS)

    target_include_directories(fuzzer-jsi-entry PRIVATE ${HERMES_SOURCE_DIR}/API)
    target_include_directories(fuzzer-jsi-entry PRIVATE /usr/local/include/libprotobuf-mutator)
    target_link_libraries(fuzzer-jsi-entry
      PRIVATE
        hermesapi
            protobuf-mutator-libfuzzer

            protobuf-mutator
            protobuf
    )
endif()
