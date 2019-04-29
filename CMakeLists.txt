cmake_minimum_required(VERSION 3.0)

aux_source_directory(. DIR_SRCS)
add_executable(infinite-chain ${DIR_SRCS})

find_package(leveldb)
find_package(Threads)
find_package(OpenSSL)
target_link_libraries(infinite-chain leveldb pthread crypto)
