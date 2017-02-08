#.rst:
# cmc-find
# --------
#
# Provides function to help writting FindXxx modules.

function(HELP_FIND module)
    set(oneValueArgs "HEADER;LIBRARY")
    #set(multiValueArgs "HEADERS;LIBRARIES")
    cmake_parse_arguments(CAS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

# The base search path
set (_${module}_PATH)

if(${module}_ROOT_DIR)
    set(_${module}_PATH ${${module}_ROOT_DIR})
elseif(${module}_ROOT)
    set(_${module}_PATH ${${module}_ROOT})
elseif(${module}_DIR)
    set(_${module}_PATH ${${module}_DIR})
endif()

##
## Look for headers and libs
##
find_path(${module}_INCLUDE_DIR ${CAS_HEADER}
          ${_${module}_PATH}/include
          #$ENV{${module}_DIR}
	  #NO_DEFAULT_PATH
	  NO_SYSTEM_ENVIRONMENT_PATH
)

#find_path(${module}_INCLUDE_DIR ${CAS_HEADER}
#    #$ENV{${module}_HOME}/include
#    #$ENV{${module}_ROOT}/include
#    /usr/local/include
#    /usr/include
#)

find_library(${module}_LIBRARY NAMES "${CAS_LIBRARY}"
    PATHS
        ${_${module}_PATH}
        ${_${module}_PATH}/lib
        #${_${module}_PATH}/lib/release
        ${_${module}_PATH}/bin
        #PATH_SUFFIXES
        #PATHbin.osx
    #NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
)

#find_library(${module}_LIBRARY NAMES ${CAS_LIBRARY}
#    PATHS
#        /usr/lib64
#        /usr/lib
#)

##	
## handle the QUIETLY and REQUIRED arguments and set ${module}_FOUND to TRUE if 
## all listed variables are TRUE
##
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(${module} 
    FOUND_VAR ${module}_FOUND
    REQUIRED_VARS ${module}_INCLUDE_DIR ${module}_LIBRARY
)

#there seems to be a bug : xxx_FOUND is always output in upper case
if (${module}_FOUND)
    set(${module}_INCLUDE_DIRS ${${module}_INCLUDE_DIR} PARENT_SCOPE)
    set(${module}_LIBRARIES ${${module}_LIBRARY} PARENT_SCOPE)
    # Unlike the 'classic' FindXxx.cmake modules, we are here in a function
    # it is thus required to forward the values to the parent scope, including the _FOUND.
    set(${module}_FOUND ${${module}_FOUND} PARENT_SCOPE)
endif()

mark_as_advanced( ${module}_INCLUDE_DIR ${module}_LIBRARY )

##
## Defines an imported target for the found library
##
if (${module}_FOUND AND NOT TARGET ${module}::${module})
    add_library(${module}::${module} UNKNOWN IMPORTED)
    set_target_properties(${module}::${module} PROPERTIES
        # \todo Why are they singular in the documentation ?
        IMPORTED_LOCATION "${${module}_LIBRARY}"
        #INTERFACE_COMPILE_OPTIONS "${PC_${module}_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${${module}_INCLUDE_DIR}"
  )
endif()

endfunction()
