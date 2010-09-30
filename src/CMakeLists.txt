set(source_files
    main.cpp
    )

add_executable(fyreware ${source_files})
target_link_libraries(fyreware
    ${QT_LIBRARIES}
    )
