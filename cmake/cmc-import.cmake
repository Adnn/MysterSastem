## CMAKE_DOCUMENTATION_START cmc_find_internal_package
##
## This macro wraps the call to find_package in CONFIG mode (internal dependencies are built as CMake packages)
## with the logic required to automatically link the imported target build to the current project build 
## if the imported target package is found in its build tree.
##
## Use OPTION_ADD_BUILDTREE_TARGETS_TO_PROJECT to control whereas the link is made by adding the dependency project files
## into the current project IDE workspace or by introducing a custom build step recompiling it on the commande line.
##
## CMAKE_DOCUMENTATION_END
macro(cmc_find_internal_package package)
    set(optionsArgs "")
    set(oneValueArgs "NAMESPACE")
    set(multiValueArgs "")
    cmake_parse_arguments(CAS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    find_package(${package} CONFIG)
    if(${package}_FOUND) #Only bother if the package was found.
        _cmc_handle_target(${package} "${CAS_NAMESPACE}")
    endif()
endmacro()


function(_cmc_handle_target target namespace)
    cmc_is_target_from_buildtree(${target} is_from_buildtree)
    if (is_from_buildtree)
        if(OPTION_ADD_BUILDTREE_TARGETS_TO_PROJECT)
            #
            # Here goes the call to scripts actually adding the imported targets' project into the generated project file.
            #
            message("(_cmc_handle_target) To be completed.")
        else()
            cmc_handle_target_origin(${target} NAMESPACE "${namespace}")
        endif()
    else() # target is imported from its install tree
        if (OPTION_VERBOSE_HANDLE_TARGET_ORIGIN)
            message(STATUS "(cmc) Internal imported target '" ${namespace}${target} "' is installed.")
        endif()
    endif()
endfunction()
