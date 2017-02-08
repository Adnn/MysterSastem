## This file is a derivative work from pre-existing code
##  Adrien David
##

## CMAKE_DOCUMENTATION_START cmc-macros.cmake
##
## Contain whole macros/functions users can use in CMakeLists.txt for handle theire tree project.
##
##  \\li see \\ref conditional_add_subdirectory.
##  \\li see \\ref list_subdirectories
##  \\li see \\ref is_empty
##
## CMAKE_DOCUMENTATION_END

if(_SYSTEM_FILES_TOOLS_CMAKE_INCLUDED_)
  return()
endif()
set(_SYSTEM_FILES_TOOLS_CMAKE_INCLUDED_ true)

cmake_minimum_required(VERSION 2.8)

include(${CMAKE_ROOT}/Modules/CMakeParseArguments.cmake)

## CMAKE_DOCUMENTATION_START define_option_variables
##
## Initialize custom variables, grouped under 'OPTION', to control CMake run.
##
## CMAKE_DOCUMENTATION_END
function(DEFINE_OPTION_VARIABLES)
    set(OPTION_VERBOSE_ADD_SUBDIR CACHE BOOL
        "When enabled, invocation of conditional_add_subdirectory() will produce a diagnostic message.")
    set(OPTION_VERBOSE_HANDLE_TARGET_ORIGIN CACHE BOOL
        "When enabled, invocation of cmc_handle_target_origin() will produce a diagnostic message.")
    set(OPTION_ADD_BUILDTREE_TARGETS_TO_PROJECT CACHE BOOL
        "When enabled, dependencies satisfied by importing a CMake target from its build tree will be added as a subproject in the generated project files. (They are otherwise automatically rebuilt by a command line step).")
    set(OPTION_SEPARATE_HEADERS_FROM_SOURCES TRUE CACHE BOOL "Grouping files in the project tree: separate headers from sources")
endfunction()

## CMAKE_DOCUMENTATION_START cmc_source_group
##
## Group files in IDEs, following a common pattern.
## By default, when given no argument, recursively groups all files starting from the current directory.
## Accept a single argument, which is optional: the relative path to a folder, where to start grouping.
##
## CMAKE_DOCUMENTATION_END
function(cmc_source_group_impl)
    if(ARGC EQUAL 1)
        set(folder "${ARGV0}")
        set(search_path "${folder}")
        # subgroup must be indicated by backslash
        string(REPLACE "/" "\\" group "${folder}")
    else()
        set(search_path ".")
    endif()

    source_group("Header Files\\${group}" REGULAR_EXPRESSION "${search_path}/.*\\.h")
    source_group("Source Files\\${group}" REGULAR_EXPRESSION "${search_path}/.*\\.(c(pp)?|m(m)?)")

    # Recursively add subfolders
    list_subdirectories(${search_path} subdirs RELATIVE)
    foreach(dir ${subdirs})
        cmc_source_group_impl(${folder}/${dir})
    endforeach()
endfunction()

macro(cmc_source_group_united)
  file(GLOB_RECURSE flist LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.*")

  foreach(fn ${flist})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${fn}")
      string(REPLACE "/" "\\" group ${fn})
      file(GLOB fcontents LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${fn}/*.*")
      source_group("${group}" FILES ${fcontents})
      #message(STATUS "source group: ${group} ${fcontents}")
    endif()
  endforeach()
endmacro()

macro(cmc_source_group)
  if(${OPTION_SEPARATE_HEADERS_FROM_SOURCES})
    cmc_source_group_impl(${ARGV0})
  else()
    cmc_source_group_united()
  endif()
endmacro()


function (cmc_in_buildtree_list target bool_result)
    list (FIND _CMC_BUILDTREE_TARGET_LIST "${target}" _index)
    #if(";${_CMC_BUILDTREE_TARGET_LIST};" MATCHES ";${target};") #Contains, see: http://stackoverflow.com/q/23323147/1027706
    if (${_index} GREATER -1)
        set(${bool_result} TRUE PARENT_SCOPE)
    else()
        set(${bool_result} FALSE PARENT_SCOPE)
    endif()
endfunction()


## CMAKE_DOCUMENTATION_START cmc_setup_output
##
## Consolidates the configuration of the debug postfix and the output directory variables used by each target install().
##
## CMAKE_DOCUMENTATION_END
function(cmc_setup_output)
    # Used to initialize DEBUG_POSTFIX property of further created targets.
    set(CMAKE_DEBUG_POSTFIX "d")

    # Potentially adds a suffix to the library installation directory, by detecting the compiler size for void*
    # Nota: copied from osg's root CMakeLists.txt
    IF(UNIX AND NOT WIN32 AND NOT APPLE)
      IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
          SET(LIB_POSTFIX "64" CACHE STRING "suffix for 32/64 dir placement")
          MARK_AS_ADVANCED(LIB_POSTFIX)
      ENDIF()
    ENDIF()
    IF(NOT DEFINED LIB_POSTFIX)
        SET(LIB_POSTFIX "")
    ENDIF()

    # On Windows platforms, .dll files are RUNTIME targets and the corresponding import library are ARCHIVE.
    # Static libraries are always treated as archive: its is not common to build a LIBRARY on Windows.
    # Those variables should be used by the install() command for the different targets.
    set(RUNTIME_OUTPUT_DIRECTORY bin PARENT_SCOPE)
    set(LIBRARY_OUTPUT_DIRECTORY lib${LIB_POSTFIX} PARENT_SCOPE)
    set(ARCHIVE_OUTPUT_DIRECTORY lib${LIB_POSTFIX} PARENT_SCOPE)
endfunction()


## CMAKE_DOCUMENTATION_START cmc_xcode_product_archive_fix
##
## This fix is required so the Product -> Archive action in Xcode places (a softlink to) the built targets in the build tree.
## Call this macro from the root CMakeLists.txt
## see: http://stackoverflow.com/q/33020245/1027706
##
## CMAKE_DOCUMENTATION_END
macro(cmc_xcode_product_archive_fix)
    if(NOT CMAKE_GENERATOR STREQUAL Xcode)
        message(WARNING "Calling 'cmc_xcode_product_archive_fix()' while the generator is not Xcode.")
    endif()
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "./")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "./")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "./")
endmacro()


## CMAKE_DOCUMENTATION_START cmc_config_dependency_code
##
## Generate the CMake script code to find the given imported target dependencies
## This code is ready to be added to a CMake package config file.
## see: http://www.cmake.org/cmake/help/v3.3/manual/cmake-packages.7.html#creating-a-package-configuration-file
##
## CMAKE_DOCUMENTATION_END
function(cmc_config_dependency_code output)
    set(optionsArgs "")
    set(oneValueArgs "")
    set(multiValueArgs "DEPENDS")
    cmake_parse_arguments(CAS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    set(_buffer "include(CMakeFindDependencyMacro)")
    foreach(dependency ${CAS_DEPENDS})
        string(CONCAT _buffer ${_buffer} "\n" "find_dependency(" ${dependency} ")")
    endforeach()

    set(${output} ${_buffer} PARENT_SCOPE)
endfunction()


function(cmc_config_buildtree_checks_code output)
    set(_check_buffer "#invokes cmc_check_import_DIR on each target imported from its build tree by the project that wrote this config.")
    foreach(dependency ${_CMC_BUILDTREE_TARGET_LIST})
        string(CONCAT _check_buffer ${_check_buffer} "\n" "    " "cmc_check_import_DIR(" ${dependency} " \"" ${${dependency}_DIR} "\")" )
    endforeach()

    set(${output} "${_check_buffer}" PARENT_SCOPE)
endfunction()


function(cmc_check_import_DIR target dir_expected)
    if(NOT "${${target}_DIR}" STREQUAL "${dir_expected}")
        message(SEND_ERROR "${target} importation directory does not match up with expectations: ${dir_expected}")
    endif()
endfunction()


function (cmc_targetfile_name projectname filename)
    set(${filename} ${projectname}Targets.cmake PARENT_SCOPE)
endfunction()


function(cmc_is_target_from_buildtree target bool_result)
    set(optionsArgs "")
    set(oneValueArgs "CACHE_DIR")
    set(multiValueArgs "")
    cmake_parse_arguments(CAS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    # Sanity check
    if (NOT ${target}_DIR)
        message(SEND_ERROR "${target}_DIR is not defined to a valid value, cannot determine ${target}'s origin.")
        return()
    endif()

    if (EXISTS "${${target}_DIR}/CMakeCache.txt")
        set(${CAS_CACHE_DIR} "${${target}_DIR}" PARENT_SCOPE)
        set(${bool_result} TRUE PARENT_SCOPE)
    else()
        set(${bool_result} FALSE PARENT_SCOPE)
    endif()
endfunction()


function(cmc_handle_target_origin imported_target)
    set(optionsArgs "VERBOSE;SILENT")
    set(oneValueArgs "NAMESPACE")
    set(multiValueArgs "")
    cmake_parse_arguments(CAS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if ((OPTION_VERBOSE_HANDLE_TARGET_ORIGIN OR CAS_VERBOSE) AND (NOT CAS_SILENT))
        set(verbose true)
    endif()

    # Sanity check
    if (NOT TARGET ${CAS_NAMESPACE}${imported_target})
        if (verbose)
            message("[cmc_handle_target_origin] No target: " ${CAS_NAMESPACE}${imported_target} ". Nothing will be done.")
        endif()
        return()
    endif()

    cmc_is_target_from_buildtree(${imported_target} is_from_buildtree CACHE_DIR _imported_cachedir)

    if (is_from_buildtree)
        load_cache(${_imported_cachedir} READ_WITH_PREFIX "_EP_" "CMAKE_HOME_DIRECTORY")
        include(ExternalProject)
        ExternalProject_Add(EP_${imported_target}
                            SOURCE_DIR ${_EP_CMAKE_HOME_DIRECTORY} # containing root CMakeLists.txt (used by configure step)
                            BINARY_DIR ${_imported_cachedir}
                            BUILD_ALWAYS 1
                            INSTALL_COMMAND "" # Disables install
        )
        add_dependencies(${CAS_NAMESPACE}${imported_target} EP_${imported_target})

        # This target joins the list of targets imported from their build tree.
        set(_CMC_BUILDTREE_TARGET_LIST ${_CMC_BUILDTREE_TARGET_LIST} ${imported_target} PARENT_SCOPE)

        if (verbose)
            message(STATUS "[cmc_handle_target_origin] Imported target " ${CAS_NAMESPACE}${imported_target} " from BUILD_TREE will automatically be regenerated.")
        endif()

    else()
        if (verbose)
            message(STATUS "[cmc_handle_target_origin] Imported target " ${CAS_NAMESPACE}${imported_target} " is installed.")
        endif()
    endif()

endfunction()


##
## This function is usefull because CMake is failing to correctly setup system include directories with Xcode
## (at least until CMake 3.3)
## see the ticket: https://public.kitware.com/Bug/view.php?id=15687
##
function(cmc_target_include_system_directories target scope include_dirs)
    if(CMAKE_GENERATOR STREQUAL Xcode)
        set(_search_path "")
        foreach(_include_dir ${include_dirs})
            set(_search_path "${_search_path} -isystem '${_include_dir}'")
        endforeach()
        separate_arguments(_search_path UNIX_COMMAND "${_search_path}") #converts to a semicolon based list
        target_compile_options(${target} ${scope} ${_search_path})
    else()
        target_include_directories(${target} SYSTEM ${scope} ${include_dirs})
    endif()
endfunction()


## CMAKE_DOCUMENTATION_START conditional_add_subdirectory
##
##  Add a subdirectory after checking the dependency found and the option set.
## The invocation will produce diagnostic message if OPTION_VERBOSE_ADD_SUBDIR is set to true, and be silent otherwise.
## This behaviour can be overrided on a per invocation basis by SILENT or VERBOSE option parameter.
##
## \\code
## CONDITIONAL_ADD_SUBDIRECTORY(<subdirectoryName> \n
##      [DEPENDS name1 [name2 ...]] \n
##      [OPTIONS name1 [name2 ...]] \n
##      [VERBOSE|SILENT]            \n
## )
## \\endcode
##  You can use without any dependencies or options to check.
## CMAKE_DOCUMENTATION_END
function(CONDITIONAL_ADD_SUBDIRECTORY subdirectory)
    set(optionsArgs "VERBOSE;SILENT")
    set(oneValueArgs "")
    set(multiValueArgs "DEPENDS;OPTIONS")
    cmake_parse_arguments(CAS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    # Determine if this invocation is verbose
    if ((OPTION_VERBOSE_ADD_SUBDIR OR CAS_VERBOSE) AND (NOT CAS_SILENT))
        set(verbose true)
    endif()

    # Prepare the status variable
    string(TOUPPER ${subdirectory} upper_directory)

    # Check options first: in case of missing option(s), do not try to satisfy dependencies
    set(missingOpt_list "") # empty list where missing options will be appended
    set(opt_list "") # list of valid options
	foreach(opt ${CAS_OPTIONS})
		if(NOT ${opt})
			list(APPEND missingOpt_list ${opt})
        else()
            list(APPEND opt_list ${opt})
		endif()
	endforeach()
	if(missingOpt_list AND verbose)
	    message(STATUS "subdirectory '${subdirectory}' will NOT be built because of missing options : ${missingOpt_list}")
	endif()

    # Check for dependencies
    if( NOT missingOpt_list) # Only go through the dependencies if all required options are ON
        set(missingDep_list "") # empty list where missing dependencies will be appended
        foreach(depend ${CAS_DEPENDS})
            string(TOUPPER ${depend} upper_name)
            # Also check if there is a target with this dependency's name:
            # if the target is created inside this cmake repo (i.e., is not imported), no '_FOUND' variable are defined
            if(NOT ${upper_name}_FOUND AND NOT ${depend}_FOUND AND NOT TARGET ${depend})
                list(APPEND missingDep_list ${depend})
            endif()
        endforeach()
        if(missingDep_list AND verbose)
            message ("subdirectory '${subdirectory}' will NOT be built because of missing dependencies : ${missingDep_list}")
        endif()
    endif()

	# add subdir if all conditins are satisfied
	if(NOT missingDep_list AND NOT missingOpt_list)
	    if(verbose)
		    message (STATUS "add_subdirectory(${subdirectory})")
		endif()
    	add_subdirectory(${subdirectory})
	endif()
endfunction()


###############################################################################


## CMAKE_DOCUMENTATION_START list_subdirectories
##  Allow to list subdirectories from the currentdir.
##  \\code
##      list_subdirectories(<currentDir> <resultListSubDirs>)
##  \\endcode
##  TODO : make a RECUSIVE option?
##
## CMAKE_DOCUMENTATION_END
macro(LIST_SUBDIRECTORIES currentDir resultListSubDirs)
    set(optionsArgs "RELATIVE")
    set(oneValueArgs "")
    set(multiValueArgs "")
    cmake_parse_arguments(CAS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )


    # Collect all file and folders from ${currentDir} in ${paths}
    if(CAS_RELATIVE)
        set(_relative_prefix "${CMAKE_CURRENT_SOURCE_DIR}/${currentDir}")
        file(GLOB paths RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${currentDir}/ "${currentDir}/*")
    else()
        set(_relative_prefix "")
        file(GLOB paths "${currentDir}/*")
    endif()

    # Keep the folders only
    set(list_of_dirs "")
    foreach(path ${paths})
        if(IS_DIRECTORY ${_relative_prefix}/${path})
            list(APPEND list_of_dirs ${path})
        endif()
    endforeach()
    set(${resultListSubDirs} ${list_of_dirs})
endmacro()


###############################################################################


## CMAKE_DOCUMENTATION_START is_empty
##  Check if a directory or a file is empty.
##  \\code
##      is_empty(<path> <resultContent>)
##  \\endcode
## usage:
## CMAKE_DOCUMENTATION_END
macro(IS_EMPTY path resultContent)
    if(IS_DIRECTORY "${path}")
        file(GLOB ${resultContent} "${path}/*")
        #message("${path} is a directory and contain : ${resultContent}")
    elseif(EXISTS "${path}")
        file(READ ${resultContent} "${path}")
        #message("${path} is a file and contain : ${resultContent}")
    endif()
endmacro()
