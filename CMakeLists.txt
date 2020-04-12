cmake_minimum_required (VERSION 3.13)

project (postgres-asio)
set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(main main.cpp)
target_include_directories(main PRIVATE include)
target_link_libraries(main PRIVATE Boost::system Threads::Threads OpenSSL::Crypto)