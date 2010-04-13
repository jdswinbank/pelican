
# Usage:
#   INCLUDE_SETUP( Subdirectory fileList )
#
# Description:
#   This mactro creates an include directory with a standard config.h inside
#   For each file in the fileList a copy is made in BUILD_INCLUDE_DIR/Subdirectory
#   Dependency targets are set for SubDirectory/filename
#
SET(BUILD_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)
FILE(MAKE_DIRECTORY ${BUILD_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${BUILD_INCLUDE_DIR})

MACRO(INCLUDE_SETUP dest)
  FILE(MAKE_DIRECTORY ${BUILD_INCLUDE_DIR}/${dest})
  FOREACH(file ${ARGN})
      GET_FILENAME_COMPONENT(filename ${file} NAME )
      SET(in_file ${CMAKE_CURRENT_BINARY_DIR}/${dest}_${filename})
      SET(out_file ${BUILD_INCLUDE_DIR}/${dest}/${filename})
      FILE(WRITE ${in_file}
          "#include \"${CMAKE_CURRENT_SOURCE_DIR}/${file}\"\n"
          )
      CONFIGURE_FILE(${in_file} ${out_file} COPYONLY)
      INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${file} DESTINATION ${INCLUDE_INSTALL_DIR}/${dest} )
  ENDFOREACH(file)
ENDMACRO(INCLUDE_SETUP dest)

# -- generate the info for config.h
INCLUDE(CheckIncludeFiles)
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)
CHECK_INCLUDE_FILES(malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES("sys/param.h;sys/mount.h" HAVE_SYS_MOUNT_H)
CHECK_INCLUDE_FILES(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES("string.h" HAVE_STRING_H)
CHECK_INCLUDE_FILES("sys/stat.h" HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES("sys/types.h" HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES( unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES( memory.h HAVE_MEMORY_H)

#
# Load in Package Dependencies
#

MACRO( PACKAGE_DEPENDENCIES )
    # add specific packages
    set(config_header "${BUILD_INCLUDE_DIR}/config.h")
    set(config_in_file "${CMAKE_CURRENT_BINARY_DIR}/config.h.in")
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/config.h.in "${config_in_file}")
    FOREACH(pack ${ARGN})
        #message( "Looking for Package ${pack}" )
        string(TOUPPER ${pack} packvar)
        FIND_PACKAGE(${pack} REQUIRED)
        IF(${packvar}_FOUND)
            FILE(APPEND ${config_in_file}
                "#define HAVE_${packvar} 1\n"
            )
        ENDIF(${packvar}_FOUND)
    ENDFOREACH(pack)
    CONFIGURE_FILE(${config_in_file} "${config_header}")
ENDMACRO( PACKAGE_DEPENDENCIES )

#
# Define a directory as a SUBPACKAGE( packageName subpackage_dependencies )
# 

MACRO( SUBPACKAGE_ADD_LIBRARIES )
    IF(SUBPACKAGE_CURRENT)
        LIST(INSERT SUBPACKAGE_LIBRARIES 0 ${ARGN})
        LIST(INSERT SUBPACKAGE_${SUBPACKAGE_CURRENT}_LIBS 0 ${ARGN})
        FILE(APPEND ${SUBPACKAGE_FILE}
            "LIST(INSERT SUBPACKAGE_${SUBPACKAGE_CURRENT}_LIBS 0 ${ARGN})\n"
            )
    ELSE(SUBPACKAGE_CURRENT)
        MESSAGE("Error: SUBPACKAGE_ADD_LIBRARIES specified outside of a SUBPACKAGE context")
    ENDIF(SUBPACKAGE_CURRENT)
ENDMACRO( SUBPACKAGE_ADD_LIBRARIES )

#
# private macro to generate the SUBPACKAGE_LIBRARIES variable
#
MACRO( SUBPACKAGE_GET_LIBS package )
    IF(NOT SUBPACKAGE_${package}_ADDED)
    FOREACH(pack ${SUBPACKAGE_${package}_DEPS} )
        SUBPACKAGE_GET_LIBS(${pack})
    ENDFOREACH(pack)
    IF(SUBPACKAGE_${package}_LIBS)
    LIST(INSERT SUBPACKAGE_LIBRARIES 0 ${SUBPACKAGE_${package}_LIBS})
    ENDIF(SUBPACKAGE_${package}_LIBS)
    SET(SUBPACKAGE_${package}_ADDED TRUE)
    ENDIF(NOT SUBPACKAGE_${package}_ADDED)
ENDMACRO( SUBPACKAGE_GET_LIBS )

# set global variables
SET(SUBPACKAGE_WORK_DIR ${CMAKE_BINARY_DIR}/_subpackages)
set(SUBPACKAGE_GLOBAL_FILE "${SUBPACKAGE_WORK_DIR}/_global.cmake")
mark_as_advanced(SUBPACKAGE_GLOBAL_FILE)
mark_as_advanced(SUBPACKAGE_CURRENT)
mark_as_advanced(SUBPACKAGE_WORK_DIR)
mark_as_advanced(SUBPACKAGE_LIBRARIES)
# write out the global package file
FILE(WRITE ${SUBPACKAGE_GLOBAL_FILE}
    "# Autogenerated file for - do not edit\n"
    "IF(PACKAGE_GLOBAL)\n"
    "RETURN()\n"
    "ENDIF(PACKAGE_GLOBAL)\n"
    )

FILE(MAKE_DIRECTORY ${SUBPACKAGE_WORK_DIR})
#
# macro to generate requirements for a SUBPACKAGE
# and load dependencies
#
MACRO( SUBPACKAGE package )
    set(SUBPACKAGE_CURRENT "${package}")

    # install target for header files
    file(GLOB public_headers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h" )
    install(FILES ${public_headers} 
            DESTINATION ${INCLUDE_INSTALL_DIR}/${package}
            )

    # process packages that it depends on
    set(SUBPACKAGE_${package}_DEPS ${ARGN})
    IF(SUBPACKAGE_${package}_DEPS)
        LIST(REVERSE SUBPACKAGE_${package}_DEPS)
    ENDIF(SUBPACKAGE_${package}_DEPS)

    set(SUBPACKAGE_FILE "${SUBPACKAGE_WORK_DIR}/${package}.cmake")
    FILE(WRITE ${SUBPACKAGE_FILE}
        "# Autogenerated file for the package : ${package} - do not edit\n"
        "IF(SUBPACKAGE_${package}_LIBS)\n"
        "RETURN()\n"
        "ENDIF(SUBPACKAGE_${package}_LIBS)\n"
        "LIST(APPEND LOFAR_PACKAGES ${package} ${ARGN})\n"
        )
    IF(SUBPACKAGE_${package}_DEPS)
        FILE(APPEND ${SUBPACKAGE_FILE}
            "SET(SUBPACKAGE_${package}_DEPS ${SUBPACKAGE_${package}_DEPS})\n"
            )
    ENDIF(SUBPACKAGE_${package}_DEPS)
    # -- all include directories defined before the macro call are exported
    IF(COMMAND GET_PROPERTY)
        get_property( includes DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES )
    ELSE(COMMAND GET_PROPERTY)
        # -- cmake 2.4 compatablity, just include everything
        set(includes ${CMAKE_INCLUDE_PATH})
    ENDIF(COMMAND GET_PROPERTY)
    FILE(APPEND ${SUBPACKAGE_GLOBAL_FILE}
            "LIST(INSERT ALL_SUBPACKAGES 0 ${SUBPACKAGE_CURRENT})\n"
            #"SUBPACKAGE_GET_LIBS(${package})\n"
            #"include_directories(${SUBPACKAGE_CURRENT})\n"
        )
    FOREACH(inc ${includes})
        FILE(APPEND ${SUBPACKAGE_FILE}
            "include_directories(${inc})\n"
            )
        FILE(APPEND ${SUBPACKAGE_GLOBAL_FILE}
            "include_directories(${inc})\n"
            )
    ENDFOREACH(inc)
    FOREACH(pack ${ARGN})
        FILE(APPEND ${SUBPACKAGE_FILE}
            "include( ${SUBPACKAGE_WORK_DIR}/${pack}.cmake )\n"
            )
    ENDFOREACH(pack)
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}) # top level package directory for includes
    FOREACH(dep ${SUBPACKAGE_${package}_DEPS} )
        include(${SUBPACKAGE_WORK_DIR}/${dep}.cmake)
        SUBPACKAGE_GET_LIBS(${dep})
    ENDFOREACH(dep)
    IF(SUBPACKAGE_LIBRARIES)
        #LIST(REMOVE_DUPLICATES SUBPACKAGE_LIBRARIES)
    ENDIF(SUBPACKAGE_LIBRARIES)
    FILE(APPEND ${SUBPACKAGE_FILE}
        "IF(LOFAR_PACKAGES)\n"
        "LIST(REMOVE_DUPLICATES LOFAR_PACKAGES)\n"
        "ENDIF(LOFAR_PACKAGES)\n"
        )
ENDMACRO( SUBPACKAGE )

#
# macro to specify the SUBPACKAGE associated library
# This will build both static and shared versions of the library
# and add the library to the SUBPACKAGE_LIBRARIES variable
#
MACRO( SUBPACKAGE_LIBRARY libname )
    IF(SUBPACKAGE_CURRENT)
        # add targets for the static and dynamic libs
        add_library("${libname}" SHARED ${ARGN})
        add_library("${libname}_static" STATIC ${ARGN})
        SET_TARGET_PROPERTIES("${libname}" PROPERTIES CLEAN_DIRECT_OUTPUT 1)
        SET_TARGET_PROPERTIES("${libname}_static" PROPERTIES OUTPUT_NAME "${libname}")
        SET_TARGET_PROPERTIES("${libname}_static" PROPERTIES PREFIX "lib")
        SET_TARGET_PROPERTIES("${libname}_static" PROPERTIES CLEAN_DIRECT_OUTPUT 1)

        IF(NOT CMAKE_BUILD_TYPE MATCHES RELEASE)
            SUBPACKAGE_ADD_LIBRARIES("${libname}")
        ELSE(NOT CMAKE_BUILD_TYPE MATCHES RELEASE)
            IF(PROJECT_LIBRARIES)
                SUBPACKAGE_ADD_LIBRARIES("${PROJECT_LIBRARIES}")
            ELSE(PROJECT_LIBRARIES)
                MESSAGE("Error: No PROJECT_LIBRARIES set")
            ENDIF(PROJECT_LIBRARIES)
        ENDIF(NOT CMAKE_BUILD_TYPE MATCHES RELEASE)
        SUBPROJECT_OBJECT_FILES("${libname}_static" "${libname}_static_objects")
        SUBPROJECT_OBJECT_FILES("${libname}" "${libname}_shared_objects")
        FILE(APPEND ${SUBPACKAGE_GLOBAL_FILE}
            "LIST(INSERT SUBPACKAGE_STATIC_LIBRARIES 0 "${libname}_static")\n"
            "LIST(INSERT SUBPACKAGE_SHARED_LIBRARIES 0 "${libname}")\n"
            "LIST(INSERT SUBPACKAGE_STATIC_OBJECTS 0 ${${libname}_static_objects})\n"
            "LIST(INSERT SUBPACKAGE_SHARED_OBJECTS 0 ${${libname}_shared_objects})\n"
            )
        # construct the global sources variable
        foreach(sourcefile ${ARGN})
            if(IS_ABSOLUTE ${sourcefile})
                LIST(APPEND PROJECT_LIBRARY_SOURCES ${sourcefile})
            else(IS_ABSOLUTE ${sourcefile})
                LIST(APPEND PROJECT_LIBRARY_SOURCES ${SUBPACKAGE_CURRENT}/${sourcefile})
            endif(IS_ABSOLUTE ${sourcefile})
        endforeach(sourcefile)
        FILE(APPEND ${SUBPACKAGE_GLOBAL_FILE}
            "LIST(APPEND PROJECT_LIBRARY_SOURCES ${PROJECT_LIBRARY_SOURCES})\n"
            )
    ELSE(SUBPACKAGE_CURRENT)
        MESSAGE("Error: SUBPACKAGE_LIBRARY specified outside of a SUBPACKAGE context")
    ENDIF(SUBPACKAGE_CURRENT)
ENDMACRO( SUBPACKAGE_LIBRARY )

#
#  Macro to create a target to build a single global library that includes
#  the contents of all the subpackage libraries (defined by SUBPACKAGE_LIBRARY)
#
MACRO( PROJECT_LIBRARY libname)
    if(CMAKE_BUILD_TYPE MATCHES RELEASE)
        include( ${SUBPACKAGE_GLOBAL_FILE} )
        # mark the object files as generated to avoid a search/missing targets
        foreach(obj ${SUBPACKAGE_SHARED_OBJECTS})
            SET_SOURCE_FILES_PROPERTIES(${obj} PROPERTIES GENERATED 1)
        endforeach(obj)
        foreach(obj ${SUBPACKAGE_STATIC_OBJECTS})
            SET_SOURCE_FILES_PROPERTIES(${obj} PROPERTIES GENERATED 1)
        endforeach(obj)
        add_library("${libname}" SHARED ${SUBPACKAGE_SHARED_OBJECTS} )
        add_library("${libname}_static" STATIC ${SUBPACKAGE_STATIC_OBJECTS} )
        SET_TARGET_PROPERTIES("${libname}_static" PROPERTIES OUTPUT_NAME "${libname}")
        SET_TARGET_PROPERTIES("${libname}_static" PROPERTIES PREFIX "lib")
        SET_TARGET_PROPERTIES("${libname}_static" PROPERTIES CLEAN_DIRECT_OUTPUT 1)
        SET_TARGET_PROPERTIES("${libname}_static" PROPERTIES  LINKER_LANGUAGE CXX)
        set_target_properties("${libname}" PROPERTIES LINKER_LANGUAGE CXX)
        add_dependencies("${libname}" ${SUBPACKAGE_SHARED_LIBRARIES})
        add_dependencies("${libname}_static" ${SUBPACKAGE_STATIC_LIBRARIES})
        #    target_link_libraries("${libname}")
        install(TARGETS "${libname}_static" DESTINATION ${LIBRARY_INSTALL_DIR})
        install(TARGETS "${libname}" DESTINATION ${LIBRARY_INSTALL_DIR})
    endif(CMAKE_BUILD_TYPE MATCHES RELEASE)
ENDMACRO( PROJECT_LIBRARY)

# ------- some utility macros
MACRO(ADD_SUFFIX rootlist suffix)
    SET(outlist)
    FOREACH(root ${${rootlist}})
        LIST(APPEND outlist ${root}${suffix})
    ENDFOREACH(root)
    SET(${rootlist} ${outlist})
ENDMACRO(ADD_SUFFIX)

MACRO(ADD_DIR_PREFIX prefix rootlist)
    SET(outlist)
    FOREACH(root ${${rootlist}})
        if(IS_ABSOLUTE ${root})
            LIST(APPEND outlist ${root})
        else(IS_ABSOLUTE ${root})
            LIST(APPEND outlist ${prefix}${root})
        endif(IS_ABSOLUTE ${root})
    ENDFOREACH(root)
    SET(${rootlist} ${outlist})
ENDMACRO(ADD_DIR_PREFIX)

#
# Macro to get the object files for a given target
#
MACRO( SUBPROJECT_OBJECT_FILES target outputObjectFiles )
    IF(SUBPACKAGE_CURRENT)
        # This hack inspired by the bug report : http://www.cmake.org/Bug/view.php?id=5155
        #
        # CMake generators are currently of 2 types: those which build single configurations, and those
        # which build multiple configurations. These 2 types use 2 different directory structures for where
        # they put their object files. The currently recommended way to deduce which type of generator
        # we're using, is to see if CMAKE_CONFIGURATION_TYPES is empty or not. If it's empty, then it's
        # single configuration. If it's non-empty, then it's multiple configuration, and contains a list of all
        # the configurations available. We're not interested in that list, only whether it's empty or non-empty.

        IF(CMAKE_CONFIGURATION_TYPES)
            # We have a multiple configuration generator. Use this directory structure.
            #
            # Note that CMAKE_BUILD_TYPE has no value when Visual Studio .sln files are generated.
            # This is because on MSVC, no build type is actually selected at generation time. The MSVC
            # user typically selects her build type after opening the .sln file. CMAKE_CFG_INTDIR expands
            # to a Visual Studio macro that will contain the right value, once Visual Studio is opened and
            # a build type is selected.
            SET(STATIC_OBJ_DIR
                ${CMAKE_CURRENT_BINARY_DIR}/${target}.dir/${CMAKE_CFG_INTDIR})
        ELSE(CMAKE_CONFIGURATION_TYPES)
            # We have a single configuration generator. Use this directory structure:
            SET(STATIC_OBJ_DIR
                ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${target}.dir)
        ENDIF(CMAKE_CONFIGURATION_TYPES)

        # Now we know what directory the objects live in. Construct the actual list of objects:
        # from the sources. We cannot glob as these files do not exist yet
        get_property( target_sources TARGET ${target} PROPERTY SOURCES )
        foreach( sourcefile ${target_sources} )
            if(IS_ABSOLUTE ${sourcefile})
                # absolutes will appear in the top level object dir
                GET_FILENAME_COMPONENT(source_name "${sourcefile}" NAME)
            else(IS_ABSOLUTE ${sourcefile})
                # relative will also be relative to the top level object dir
                set(source_name "${sourcefile}")
            endif(IS_ABSOLUTE ${sourcefile})
            LIST(APPEND ${outputObjectFiles} ${source_name}${CMAKE_C_OUTPUT_EXTENSION})
        endforeach( sourcefile )
        ADD_DIR_PREFIX(${STATIC_OBJ_DIR}/ ${outputObjectFiles}) 
        #MESSAGE("${SUBPACKAGE_CURRENT}: OBJECT=${${outputObjectFiles}}")
    ELSE(SUBPACKAGE_CURRENT)
        MESSAGE("Error: SUBPACKAGE_LIBRARY specified outside of a SUBPACKAGE context")
    ENDIF(SUBPACKAGE_CURRENT)
ENDMACRO( SUBPROJECT_OBJECT_FILES )


