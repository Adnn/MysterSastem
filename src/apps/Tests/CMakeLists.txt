## #################################################################
## CMakeLists.txt
## #################################################################

project(Tests)

set(${PROJECT_NAME}_HEADERS
)

set(${PROJECT_NAME}_SOURCES
)

add_executable(${PROJECT_NAME}
    main.cpp
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
)


target_include_directories(${PROJECT_NAME} PRIVATE
)

target_link_libraries(${PROJECT_NAME}
    Components
)


######################################################################
## Installation stuff - common to all projects
######################################################################

set_target_properties(${PROJECT_NAME} PROPERTIES
                      VERSION "${VERSION}"
                      SOVERSION "${SOVERSION}"
)

# Install the executable
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${RUNTIME_OUTPUT_DIRECTORY}
        LIBRARY DESTINATION ${LIBRARY_OUTPUT_DIRECTORY}
        ARCHIVE DESTINATION ${ARCHIVE_OUTPUT_DIRECTORY}
)
