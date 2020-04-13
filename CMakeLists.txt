cmake_minimum_required (VERSION 3.13)

include(FetchContent)

project (postgres-asio)
set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Date
FetchContent_Declare(
    date
    GIT_REPOSITORY https://github.com/HowardHinnant/date.git
    GIT_TAG        v2.4.1
)

FetchContent_GetProperties(date)
if(NOT date_POPULATED)
    FetchContent_Populate(date)
    # Building date requires CMake 3.14, and we only need headers
endif()

add_executable(main main.cpp)
target_include_directories(main PRIVATE include ${date_SOURCE_DIR}/include)
target_link_libraries(main PRIVATE Boost::system Threads::Threads OpenSSL::Crypto)

