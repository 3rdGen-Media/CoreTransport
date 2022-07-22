#
# Establish Version Limits to a minimum of 3.17 for CMake's Build Systems.
#
#cmake_minimum_required (VERSION 3.20)
project(CoreTransport LANGUAGES CXX C)

#
# Establish this Library Target
#

# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.
add_library (
        CXTransport STATIC
        CXTransport/CXConnection.cpp
        CXTransport/CXCursor.cpp
        CXTransport/CXQueue.cpp
        CXURL/CXURLCursor.cpp
        CXURL/CXURLRequest.cpp
        CXURL/CXURLSession.cpp
        CXURL/CXURLInterface.cpp
        CXReQL/CXReQLCursor.cpp
        CXReQL/CXReQLQuery.cpp
        CXReQL/CXReQLSession.cpp
        CXReQL/CXReQLInterface.cpp
)

#Tell CMake to use custom name for generated target lib/exe
#set_target_properties(CTransport PROPERTIES OUTPUT_NAME "CXTransport")
#OR instead just suppress the "lib" prefix attached to static libs
set_target_properties(CXTransport PROPERTIES PREFIX "")

#Set a custom postfix for the target build config type
set_target_properties(CXTransport PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

#CMAKE static libs output dirs are controlled using ARCHIVE_OUTPUT_DIRECTORY
#CMAKE shared libs output dirs are controlled using LIBRARY_OUTPUT_DIRECTORY
set_target_properties(CXTransport PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(CXTransport PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#set_target_properties(CXTransport PROPERTIES RELEASE_POSTFIX ${CMAKE_DEBUG_POSTFIX})

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (CXTransport PUBLIC ../include/ ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (CXTransport PUBLIC ../include/CoreTransport ${CMAKE_CURRENT_SOURCE_DIR})
#target_include_directories (CXTransport PUBLIC ../include/rapidjson ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries (CXTransport LINK_PUBLIC dill CAuth CTransport)

#Add a post build step that will copy include files to directory structure that should be utilized by projects that link CTransport
add_custom_command(TARGET CXTransport POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CXTransport/*.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/CoreTransport/CXTransport
    COMMENT "Copying CXTransport Source Headers to CoreTransport/include/CXTransport directory."
)

add_custom_command(TARGET CXTransport POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CXURL/*.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/CoreTransport/CXURL
    COMMENT "Copying CXURL Source Headers to CoreTransport/include/CXURL directory."
)

add_custom_command(TARGET CXTransport POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CXReQL/*.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/CoreTransport/CXReQL
    COMMENT "Copying CXReQL Source Headers to CoreTransport/include/CXReQL directory."
)

add_custom_command(TARGET CXTransport POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CXTransport.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/CoreTransport
    COMMENT "Copying CXTransport.h to CoreTransport/include directory."
)

add_custom_command(TARGET CXTransport POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CXURL.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/CoreTransport
    COMMENT "Copying CXURL.h to CoreTransport/include directory."
)

add_custom_command(TARGET CXTransport POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CXReQL.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/CoreTransport
    COMMENT "Copying CXReQL.h to CoreTransport/include directory."
)
