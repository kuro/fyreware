cmake_minimum_required(VERSION 2.8)

project(Fyreware)

find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL QtScript REQUIRED)
include(${QT_USE_FILE})

subdirs(src)
