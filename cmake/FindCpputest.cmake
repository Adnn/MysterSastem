#.rst:
# FindCpputest
# ---------
#
# Find Cpputest include dirs and libraries
#
# Use this module by invoking find_package with the form::
#
#   find_package(Cpputest
#                [REQUIRED]     # Fail with error if Cpputest is not found
#   )                      
#
# This module finds headers and libraries.
# Results are made available through the imported target::
#
#   Cpputest::Cpputest
#
# Results are alternatively reported in variables::
#
#   Cpputest_FOUND            - True if headers and requested libraries were found
#   Cpputest_INCLUDE_DIRS     - Cpputest include directories
#   Cpputest_LIBRARY_DIRS     - Link directories for Cpputest libraries
#
# This module reads hints about search locations from variables::
#
#   Cpputest_ROOT_DIR         - Preferred installation prefix
#    (or Cpputest_ROOT)


include(cmc-find)
help_find(Cpputest HEADER CppUTest/TestHarness.h LIBRARY CppUTest)

 ## Winmm library should be added on Windows
if(Cpputest_FOUND AND WIN32)
    set(Cpputest_LIBRARIES ${Cpputest_LIBRARIES} Winmm.lib)     
endif()
