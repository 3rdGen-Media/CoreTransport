#
# Establish Version Limits to a minimum of 3.17 for CMake's Build Systems.
#
#cmake_minimum_required (VERSION 3.20)

#
# Establish this Library Target
#
#project(VitalTransport LANGUAGES CXX C)

# Create a library called "CAuth" which includes the source file "ca_scram.c".
# The extension is already found. Any number of sources could be listed here.
add_library (
        CAuth STATIC
        CAuth/ca_scram.c        
)

#Tell CMake to use custom name for generated target lib/exe
#set_target_properties(CAuth PROPERTIES OUTPUT_NAME "CAuth")
#OR instead just suppress the "lib" prefix attached to static libs
set_target_properties(CAuth PROPERTIES PREFIX "")

#Set a custom postfix for the target build config type
set_target_properties(CAuth PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

#CMAKE static libs output dirs are controlled using ARCHIVE_OUTPUT_DIRECTORY
#CMAKE shared libs output dirs are controlled using LIBRARY_OUTPUT_DIRECTORY
set_target_properties(CAuth PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set_target_properties(CAuth PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

#set_target_properties(CTransport PROPERTIES RELEASE_POSTFIX ${CMAKE_DEBUG_POSTFIX})

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
#target_include_directories (VAuth PRIVATE ../../ThirdParty/asio/include ${CMAKE_CURRENT_SOURCE_DIR})
#target_include_directories (VAuth PUBLIC ../../ThirdParty/rapidjson ${CMAKE_CURRENT_SOURCE_DIR})

#Add a post build step that will copy include files to directory structure that should be utilized by projects that link VTransport
add_custom_command(TARGET CAuth POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CAuth/*.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/CoreTransport/CAuth
    COMMENT "Copying CAuth Source Headers to CoreTransport/include/ directory."
)

