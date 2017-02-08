find_package(Doxygen)

if (NOT DOXYGEN_FOUND) 
    message(SEND_ERROR "Doxygen not found, unable to generate documentation")
endif (NOT DOXYGEN_FOUND) 

set (DOXYCONF_INPUT "${CMAKE_SOURCE_DIR}/src" CACHE STRING "Folders to be documented, searched recursively.")
set (DOXYCONF_OUTPUT ${CMAKE_BINARY_DIR}/doxygen-output CACHE PATH "Directory receiving the generated documentation.")
set (DOXYCONF_INSTALL ${PROJECT_NAME}/documentation CACHE PATH "Directory where the documentation will be installed.")

configure_file(
    Doxyfile.in
    ${DOXYCONF_OUTPUT}/Doxyfile
    @ONLY
)

add_custom_target(
        doc
        ${DOXYGEN_EXECUTABLE} ${DOXYCONF_OUTPUT}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Building documentation..."
)

if(VERBOSE)
    message(STATUS "***INFO***: You can use doc target to build documentation")
endif()

install (DIRECTORY ${DOXYCONF_OUTPUT}/html/ DESTINATION ${DOXYCONF_INSTALL})
