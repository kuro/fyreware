cmake_minimum_required(VERSION 2.8)

project(Fyreware)

find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL QtScript REQUIRED)
include(${QT_USE_FILE})

find_package(FMOD REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Cg REQUIRED)
find_package(Bullet REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (CMAKE_COMPILER_IS_GNUCC)
    add_definitions(-Wall -Werror -Wextra)
endif ()

subdirs(
    ext/qtfmod/src
    src
    )
