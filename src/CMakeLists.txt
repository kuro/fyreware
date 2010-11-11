set(source_files
    defs.h
    main.cpp
    Camera.h
    Camera.cpp
    Cluster.h
    Cluster.cpp
    OrbitalCamera.h
    OrbitalCamera.cpp
    Scene.h
    Scene.cpp
    ShaderProgram.h
    ShaderProgram.cpp
    Shader.h
    Shader.cpp
    Shell.h
    Shell.cpp

    ui/Playlist.h
    ui/Playlist.cpp
    ui/DirectoryScanner.h
    ui/DirectoryScanner.cpp
    ui/PlaylistModel.h
    ui/PlaylistModel.cpp
    ui/Player.h
    ui/Player.cpp

    ui/GraphicsView.h
    ui/GraphicsView.cpp
    )

include_directories(
    ${FMOD_INCLUDE_DIRS}
    ${QtFMOD_SOURCE_DIR}
    ${CG_INCLUDE_DIR}
    ${BULLET_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    )

qt4_add_resources(resource_files
    fyreware.qrc
    )

qt4_wrap_ui(ui_files
    ui/Playlist.ui
    ui/Player.ui
    )

qt4_automoc(${source_files})

add_executable(fyreware ${source_files} ${resource_files} ${ui_files})
target_link_libraries(fyreware
    QtFMOD
    ${GLEW_LIBRARY}
    ${FMOD_LIBRARY}
    ${QT_LIBRARIES}
    ${CG_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${BULLET_LIBRARIES}
    )
