## #################################################################
## CMakeLists.txt
## #################################################################

project(Components)

# TODO : list .dox standalone doxygen pages here
set(${PROJECT_NAME}_DOXYGEN
    #mainpage.dox
)

# TODO : list .h files here. 
# Helpful windows command : dir /b /oen
set(${PROJECT_NAME}_HEADERS
    AddressingModes.h
    Memory.h
    RegisterSet.h
    z80.h
)

# TODO : list .cpp files here
set(${PROJECT_NAME}_SOURCES
    z80.cpp
)

cmc_source_group() ## For IDE generators, groups files based on the subfolder organisation

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_DOXYGEN}
)


## Adds the library headers themselves to the interface include directory
target_include_directories(${PROJECT_NAME} INTERFACE 
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
                           $<INSTALL_INTERFACE:include>
)

## Adds the parent folder to the header search path used when building *this* library.
## It allows this library's code to include other headers from this library using <${PROJECT_NAME}/file.h>
## (instead of relying on potentially backtracing relative paths)
## Drawback: this is making the other libraries in the lib/ folder visible for inclusion.
target_include_directories(${PROJECT_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

# Public includes
target_include_directories(${PROJECT_NAME} PUBLIC
)

# Public includes treated as system (i.e., not generating warnings)
#cmc_target_include_system_directories(${PROJECT_NAME} PUBLIC
#)

target_link_libraries(${PROJECT_NAME}
)


######################################################################
## Installation stuff - common to all projects
######################################################################

set_target_properties(${PROJECT_NAME} PROPERTIES
                      VERSION "${VERSION}"
                      SOVERSION "${SOVERSION}"
)

# Generate the export set for the target
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets
        RUNTIME DESTINATION ${RUNTIME_OUTPUT_DIRECTORY}
        LIBRARY DESTINATION ${LIBRARY_OUTPUT_DIRECTORY}
        ARCHIVE DESTINATION ${ARCHIVE_OUTPUT_DIRECTORY}
)

# Install the headers
    ## Note: Does install all files, but does *not* recreate the folder structure
install(FILES ${${PROJECT_NAME}_HEADERS}
        DESTINATION include/${PROJECT_NAME})
    
    ## Alternatively: Install a subdir content
#install(DIRECTORY ${_subdir}/ DESTINATION include/${PROJECT_NAME}/${_subdir}
#        FILES_MATCHING PATTERN "*.h")


# TODO: When including internal dependencies using 'cmc_find_internal_package()', those internal dependencies
# linked against by this target should be listed in 'cmc_config_dependency_code()'.
# List dependencies to be found by the config file, and export the target.
#cmc_config_dependency_code(TARGET_DEPENDENCIES DEPENDS __ __)

include(cmc-export)
# TODO: set namespace
cmc_export(${PROJECT_NAME} NAMESPACE ad::)
