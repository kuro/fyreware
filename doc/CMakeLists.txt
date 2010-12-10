
find_package(Doxygen)

add_custom_target(doc)

execute_process(
    COMMAND ./versionfilter.rb
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE PROJECT_NUMBER
    )

configure_file(
    common.dox.in
    ${CMAKE_CURRENT_BINARY_DIR}/common.dox
    )

foreach(project FyreWare)
    set(PROJECT_NAME ${project})
    configure_file(
        ${project}.dox.in
        ${CMAKE_CURRENT_BINARY_DIR}/${project}.dox
        )

    if (DOXYGEN_FOUND)
        add_custom_target(
            ${project}-doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/${project}.dox
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
        add_dependencies(doc ${project}-doc)
    endif()
endforeach()
