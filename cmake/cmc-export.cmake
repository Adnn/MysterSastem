## CMAKE_DOCUMENTATION_START cmc_export
##
## cmc_export is a macro wrapping the boilerplate code that is used to install an export
## created by install(TARGET ... EXPORT ...).
## See: http://www.cmake.org/cmake/help/v3.0/manual/cmake-packages.7.html#creating-packages for the source of this code
##
## CMAKE_DOCUMENTATION_END
macro(cmc_export project_name)
    set(oneValueArgs "NAMESPACE")
    cmake_parse_arguments(CAS "" "${oneValueArgs}" "" ${ARGN})

    # TARGETFILE is used in Config file as well as in this macro
    cmc_targetfile_name(${project_name} TARGETFILE)

    # Generate config files in the build tree.
    configure_file(${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in
                   ${CMAKE_BINARY_DIR}/${project_name}Config.cmake
                   @ONLY)

    cmc_config_buildtree_checks_code(BUILD_TREE_DIR_CHECKS)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/Config-BuildTreeChecks.cmake.in
                   ${CMAKE_BINARY_DIR}/${project_name}Config-BuildTreeChecks.cmake
                   @ONLY)


    # Custom variable holding the destination folder for package configuration files
    set (ConfigPackageLocation cmake/)

    # Install the config file (not the BuildTreeChecks.cmake, which is build tree only)
    install(FILES ${CMAKE_BINARY_DIR}/${project_name}Config.cmake
            DESTINATION ${ConfigPackageLocation})

    # Exports
    install(EXPORT ${project_name}Targets NAMESPACE "${CAS_NAMESPACE}"
            DESTINATION ${ConfigPackageLocation}) #install tree
    export(EXPORT ${project_name}Targets NAMESPACE "${CAS_NAMESPACE}" 
           FILE ${CMAKE_BINARY_DIR}/${TARGETFILE}) #build tree
endmacro()
