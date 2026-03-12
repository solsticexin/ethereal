cmake_minimum_required(VERSION 4.1)

project(eirian VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
foreach(config Debug Release RelWithDebInfo MinSizeRel)
    set("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config}" "${PROJECT_SOURCE_DIR}/bin")
endforeach()

find_package(spdlog CONFIG QUIET)
if(NOT spdlog_FOUND)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
    )
    FetchContent_MakeAvailable(spdlog)
endif()

add_library(eirian
        src/Epoll.cpp
        src/InetAddress.cpp
        src/logger.cpp
        src/socket.cpp
        src/Buffer.cpp
        src/Channel.cpp
        src/EventLoop.cpp
        src/TcpConnection.cpp
        src/TcpServer.cpp
        src/EventLoopThread.cpp
)

target_include_directories(eirian
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
target_compile_features(eirian PUBLIC cxx_std_20)
target_link_libraries(eirian PUBLIC spdlog::spdlog_header_only)

add_executable(eirian_app main.cpp)
target_link_libraries(eirian_app PRIVATE eirian)

option(EIRIAN_BUILD_TESTS "Build test executables" ON)

if(EIRIAN_BUILD_TESTS)
    enable_testing()
    find_package(Threads REQUIRED)

    function(eirian_add_test source_file)
        get_filename_component(test_name "${source_file}" NAME_WE)
        add_executable("${test_name}" "${source_file}")
        target_link_libraries("${test_name}" PRIVATE eirian Threads::Threads)
        add_test(NAME "${test_name}" COMMAND "${test_name}")
    endfunction()

    file(GLOB test_sources CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/test/*.cpp"
    )

    foreach(test_source IN LISTS test_sources)
        eirian_add_test("${test_source}")
    endforeach()
endif()
