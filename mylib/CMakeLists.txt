cmake_minimum_required(VERSION 3.2)
project(mylib)

set(SOURCES
    logMsg.c
    logTest.c
)

add_library(mylib STATIC ${SOURCES})
