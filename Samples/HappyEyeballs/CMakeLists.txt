#
# Establish Version Limits to a minimum of 3.17 for CMake's Build Systems.
#
#cmake_minimum_required (VERSION 3.20)

#
# Establish this Executable Target
#

# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.
add_executable (HappyEyeballs HappyEyeballs.c)
target_link_directories(HappyEyeballs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../bin)
target_link_directories(HappyEyeballs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../Git/libdispatch)

#Tell CMake to use custom name for generated target lib/exe
#set_target_properties(CTransport PROPERTIES OUTPUT_NAME "CTransport")
#OR instead just suppress the "lib" prefix attached to static libs
set_target_properties(HappyEyeballs PROPERTIES PREFIX "")

#Set a custom postfix for the target build config type
set_target_properties(HappyEyeballs PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

#CMAKE static libs output dirs are controlled using ARCHIVE_OUTPUT_DIRECTORY
#CMAKE shared libs output dirs are controlled using LIBRARY_OUTPUT_DIRECTORY
set_target_properties(HappyEyeballs PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Make sure the compiler can find include files needed by our executable
target_include_directories (HappyEyeballs PRIVATE ../../include ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (HappyEyeballs PRIVATE ../../../Git/libdispatch ${CMAKE_CURRENT_SOURCE_DIR})

#Link our executable with -libpthreads
find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  set_property(TARGET HappyEyeballs PROPERTY COMPILE_OPTIONS "-pthread")
  set_property(TARGET HappyEyeballs PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(HappyEyeballs LINK_PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
endif()

#Link our executable with CTransport
target_link_libraries (HappyEyeballs LINK_PUBLIC CTransport)

#Link our executable with CAuth
target_link_libraries (HappyEyeballs LINK_PUBLIC CAuth)

#Link our executable with dill
target_link_libraries (HappyEyeballs LINK_PUBLIC dill)

#Link our executable with Clang libBlocksRuntime
target_link_libraries(HappyEyeballs PUBLIC BlocksRuntime)

target_link_libraries (HappyEyeballs PUBLIC dispatch)


