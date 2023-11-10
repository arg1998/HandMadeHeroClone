cmake_minimum_required(VERSION 3.26)

project(HandMadeHero 
        VERSION 1.0 
        LANGUAGES CXX)

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 "src/Win32_main.cpp") 
endif(WIN32)

if(LINUX)
    add_executable(${PROJECT_NAME} "src/Linux_main.cpp")
endif(LINUX)


