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
        CTransport STATIC
        CTransport/CTFile.c
        CTransport/CTSocket.c
        CTransport/CTSSL.c
        CTransport/CTransportAPI.c
        CTransport/CTQueue.c
        CTransport/CTReQL.c
)

#Tell CMake to use custom name for generated target lib/exe
#set_target_properties(CTransport PROPERTIES OUTPUT_NAME "CTransport")
#OR instead just suppress the "lib" prefix attached to static libs
set_target_properties(CTransport PROPERTIES PREFIX "")

#Set a custom postfix for the target build config type
set_target_properties(CTransport PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

#CMAKE static libs output dirs are controlled using ARCHIVE_OUTPUT_DIRECTORY
#CMAKE shared libs output dirs are controlled using LIBRARY_OUTPUT_DIRECTORY
set_target_properties(CTransport PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(CTransport PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#set_target_properties(CTransport PROPERTIES RELEASE_POSTFIX ${CMAKE_DEBUG_POSTFIX})

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (CTransport PUBLIC ../include/ ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (CTransport PUBLIC ../include/CoreTransport ${CMAKE_CURRENT_SOURCE_DIR})
#target_include_directories (CTransport PRIVATE ../../ThirdParty/asio/include ${CMAKE_CURRENT_SOURCE_DIR})
#target_include_directories (CTransport PUBLIC ../../ThirdParty/rapidjson ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries (CTransport LINK_PUBLIC dill CAuth)

#Add a post build step that will copy include files to directory structure that should be utilized by projects that link CTransport
add_custom_command(TARGET CTransport POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CTransport/*.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/CoreTransport/CTransport
    COMMENT "Copying CTransport Source Headers to CoreTransport/include/CTransport directory."
)

add_custom_command(TARGET CTransport POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/*.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/CoreTransport/protobuf
    COMMENT "Copying Protobuf Source Headers to CoreTransport/include/CTransport directory."
)

add_custom_command(TARGET CTransport POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CTransport.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/CoreTransport
    COMMENT "Copying CoreTransport.h to CoreTransport/include directory."
)
