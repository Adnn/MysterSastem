## CMAKE_DOCUMENTATION_START enable_cpp_version
##
## Initialize custom variables, grouped under 'OPTION', to control CMake run.
##
## CMAKE_DOCUMENTATION_END
function(ENABLE_CPP_VERSION)
    set(optionsArgs "C++11;C++14")
    cmake_parse_arguments(CAS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if (CAS_C++11)
        if(UNIX)
            # Nota: list(APPEND...) does not work: it creates a scope local variable.
            set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -stdlib=libc++" PARENT_SCOPE)
        endif(UNIX)
    elseif(CAS_C++14)
        if(UNIX)
            # Nota: list(APPEND...) does not work: it creates a scope local variable.
            set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++1y -stdlib=libc++" PARENT_SCOPE)
        endif(UNIX)
    else()
        message ("enable_cpp_version() must be invoked with a valid langage version.")
        return()
    endif()

    # /todo Only specify the library for version of XCode that do not use the right one by default.
    #With C++14 support, Xcode should use libc++ (by Clang) instead of default GNU stdlibc++
    if (CMAKE_GENERATOR STREQUAL "Xcode")
        SET(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++" PARENT_SCOPE)
        SET(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++1y" PARENT_SCOPE)
    endif ()

endfunction()
