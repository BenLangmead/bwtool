cmake_minimum_required(VERSION 3.11)
project(bwtool_langmead C)

set(CMAKE_C_STANDARD 11)

include_directories(.)
add_definitions(-DPACKAGE_STRING="bwtool-dev")
find_package(Threads)

add_executable(bwtool_langmead
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

target_link_libraries(bwtool_langmead beato jkweb m z hts ${CMAKE_THREAD_LIBS_INIT})
