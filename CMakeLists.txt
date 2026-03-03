    cmake_minimum_required(VERSION 3.20)
    project(ZetaXDP LANGUAGES C CXX)

    if (NOT UNIX OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
      message(FATAL_ERROR "ZetaXDP is Linux-only.")
    endif()

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    option(ZETAXDP_BUILD_XDP "Build kernel-side XDP program (requires clang)" ON)

    # Warnings
    add_compile_options(-Wall -Wextra -Wpedantic)

    # libbpf via pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBBPF REQUIRED libbpf)

    add_library(zetaxdp_common INTERFACE)
    target_include_directories(zetaxdp_common INTERFACE
      ${LIBBPF_INCLUDE_DIRS}
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_directories(zetaxdp_common INTERFACE ${LIBBPF_LIBRARY_DIRS})
    target_link_libraries(zetaxdp_common INTERFACE ${LIBBPF_LIBRARIES})
    target_compile_definitions(zetaxdp_common INTERFACE _GNU_SOURCE)

    # Receiver
    add_executable(zetaxdp
      src/main.cpp
      src/cli.cpp
      src/util.cpp
      src/stats.cpp
      src/csv.cpp
      src/seq.cpp
      src/mode_socket.cpp
      src/mode_xdp.cpp
    )
    target_link_libraries(zetaxdp PRIVATE zetaxdp_common pthread rt)

    # Generator
    add_executable(zetagen
      tools/generator.cpp
      src/util.cpp
      src/cli_gen.cpp
    )
    target_include_directories(zetagen PRIVATE include)
    target_link_libraries(zetagen PRIVATE pthread rt)
target_compile_definitions(zetagen PRIVATE _GNU_SOURCE)

    # Embed git SHA if available
    find_program(GIT_EXECUTABLE git)
    if (GIT_EXECUTABLE)
      execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_SHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
      )
      if (GIT_SHA)
        target_compile_definitions(zetaxdp PRIVATE ZETAXDP_GIT_SHA="${GIT_SHA}")
        target_compile_definitions(zetagen PRIVATE ZETAXDP_GIT_SHA="${GIT_SHA}")
      endif()
    endif()

    # Build XDP program object
    if (ZETAXDP_BUILD_XDP)
      find_program(CLANG_EXECUTABLE clang)
      if (NOT CLANG_EXECUTABLE)
        message(WARNING "clang not found; disabling XDP build. (set -DZETAXDP_BUILD_XDP=OFF to silence)")
        set(ZETAXDP_BUILD_XDP OFF)
      endif()
    endif()

    if (ZETAXDP_BUILD_XDP)
      set(XDP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/xdp/zeta_xdp_kern.c)
      set(XDP_OBJ ${CMAKE_CURRENT_BINARY_DIR}/zeta_xdp_kern.o)

      add_custom_command(
        OUTPUT ${XDP_OBJ}
        COMMAND ${CMAKE_COMMAND} -E echo "Building XDP program: ${XDP_OBJ}"
        COMMAND ${CLANG_EXECUTABLE}
          -O2 -g -target bpf
          -D__TARGET_ARCH_x86
          -I${CMAKE_CURRENT_SOURCE_DIR}/xdp
          -c ${XDP_SRC} -o ${XDP_OBJ}
        DEPENDS ${XDP_SRC}
        VERBATIM
      )
      add_custom_target(xdp_obj ALL DEPENDS ${XDP_OBJ})

      # Install/copy object next to binaries for convenience
      add_custom_command(TARGET xdp_obj POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/xdp
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${XDP_OBJ} ${CMAKE_CURRENT_BINARY_DIR}/xdp/zeta_xdp_kern.o
      )

      target_compile_definitions(zetaxdp PRIVATE ZETAXDP_DEFAULT_XDP_OBJ_PATH="${CMAKE_CURRENT_BINARY_DIR}/xdp/zeta_xdp_kern.o")
    endif()
