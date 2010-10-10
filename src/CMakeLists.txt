set(source_files
    defs.h
    main.cpp
    Scene.h
    Scene.cpp
    )

include_directories(
    ${FMOD_INCLUDE_DIRS}
    ${QtFMOD_SOURCE_DIR}
    )

qt4_automoc(${source_files})

add_executable(fyreware ${source_files})
target_link_libraries(fyreware
    QtFMOD
    ${FMOD_LIBRARY}
    ${QT_LIBRARIES}
    ${OPENGL_LIBRARIES}
    )
