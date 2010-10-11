set(source_files
    defs.h
    main.cpp
    Camera.h
    Camera.cpp
    Scene.h
    Scene.cpp
    ShaderProgram.h
    ShaderProgram.cpp
    Shader.h
    Shader.cpp
    )

include_directories(
    ${FMOD_INCLUDE_DIRS}
    ${QtFMOD_SOURCE_DIR}
    ${CG_INCLUDE_DIR}
    ${BULLET_INCLUDE_DIR}
    )

qt4_automoc(${source_files})
qt4_add_resources(resource_files fyreware.qrc)

add_executable(fyreware ${source_files} ${resource_files})
target_link_libraries(fyreware
    QtFMOD
    ${FMOD_LIBRARY}
    ${QT_LIBRARIES}
    ${CG_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${BULLET_LIBRARIES}
    )
