## #################################################################
## Root CMakeLists.txt
## #################################################################

# Not tested with previous versions
cmake_minimum_required(VERSION 3.0)

project(SMS)


## #################################################################
## Setup CMake Modules
## #################################################################

# Where to look for custom FindXXX.cmake files and custom includes
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Definitions of custom macros
include (cmc-macros)

# Definition of custom variables
define_option_variables()


## #################################################################
## Setup output paths
## #################################################################

# Setups the debug postfix, and populates *_OUTPUT_DIRECTORY variables.
cmc_setup_output()

if(CMAKE_GENERATOR STREQUAL Xcode)
    # Allows to execute the Product -> Archive action in Xcode.
    cmc_xcode_product_archive_fix()
endif()


## #################################################################
## Build options
## #################################################################

# Doxygen documentation generation
option(BUILD_DOCUMENTATION "Build doxygen's documentation" false)
conditional_add_subdirectory(cmake/doc OPTIONS BUILD_DOCUMENTATION)

# CMake automatically checks this variable value when calling add_library command.
option(BUILD_SHARED_LIBS "Build shared libraries" false)

#
# Select modules to build
#

# TODO : list optional components here, assigning one variable each
#option (BUILD_xxx "Build xxx application" true)


# 
# C++14 support
# 
set(CMAKE_CXX_STANDARD 14)


## #################################################################
## Resolve dependencies
## #################################################################

# TODO : find all your dependencies packages

#
# Internal dependencies
#
include(cmc-import)                    
#cmc_find_internal_package(__   NAMESPACE __::)


#
# 3rd party dependencies
#

#find_package(__)


## #################################################################
## Add targets
## #################################################################

# Start recursive crawling of the sources
add_subdirectory(src)
