cmake_minimum_required(VERSION 3.12)
project(p3 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(p3
        tests/align.c
        tests/align2.c
        tests/align3.c
        tests/alloc.c
        tests/alloc2.c
        tests/alloc2_nospace.c
        tests/alloc3.c
        tests/alloc_nospace.c
        tests/bestfit.c
        tests/coalesce1.c
        tests/coalesce2.c
        tests/coalesce3.c
        tests/coalesce4.c
        tests/coalesce5.c
        tests/coalesce6.c
        tests/free2.c
        tests/free3.c
        tests/writeable.c
        mem.c
        mem.h)
