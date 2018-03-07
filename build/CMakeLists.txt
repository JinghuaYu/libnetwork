cmake_minimum_required(VERSION 2.8)
PROJECT(libnetwork)

# treat warnings as errors as more as possible
add_definitions("-O3 -Wfatal-errors -Wall -Wextra -Werror -Wno-missing-field-initializers")

SET(SRC_LIST ../libnetwork.c)
SET(SENDMAV_BINARY_DIR ../)
SET(SENDMAV_SOURCE_DIR ${SRC_LIST})
MESSAGE(STATUS "This is BINARY dir "${SENDMAV_BINARY_DIR})
MESSAGE(STATUS "This is SOURCE dir "${SENDMAV_SOURCE_DIR})
include_directories(
)
ADD_LIBRARY(network SHARED ${SRC_LIST})
