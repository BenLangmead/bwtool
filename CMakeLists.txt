cmake_minimum_required(VERSION 3.13)
project(bwtool_langmead C)

set(CMAKE_C_STANDARD 11)

include_directories(.)
add_compile_definitions(PACKAGE_STRING="bwtool-dev")

add_executable(bwtool_langmead
        tests/bwmake.c
        tests/wigmake.c
        aggregate.c
        bwtool.c
        bwtool.h
        bwtool_shared.c
        bwtool_shared.h
        chromgraph.c
        distrib.c
        extract.c
        fill.c
        find.c
        lift.c
        matrix.c
        paste.c
        rand.c
        remove.c
        roll.c
        sax.c
        shift.c
        split.c
        summarize.c
        window.c)
