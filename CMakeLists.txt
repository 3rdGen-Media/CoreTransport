# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 3.20)

# Set Workspace/Solution Level Options
# Turn on WolfSSL for CTransport TLS connections and define associated preprocessor definition
add_definitions(-DCTRANSPORT_WOLFSSL)
option(CTRANSPORT_WOLFSSL "Builds CTransport with WolfSSL TLS Support for Secure Connections" ON)
add_definitions(-DWOLFSSL_DEBUG)

set (CMAKE_C_FLAGS "-fblocks")
set (CMAKE_CXX_FLAGS "-fblocks")
set (CMAKE_SHARED_LINKER_FLAGS "-lBlocksRuntime")

message(STATUS "CoreTransport CMAKE_CXX_COMPILER_ID is ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CoreTrasnport CMAKE_CXX_COMPILER_VERSION is ${CMAKE_CXX_COMPILER_VERSION}")

#Define projects for top level CMakeLists.txt (ie Workspace CmakeList) to build
#project(dill LANGUAGES CXX C)
#project(CAuth LANGUAGES CXX C)
#project(CTransport LANGUAGES CXX C)
#project(CXTransport LANGUAGES CXX C)
#project(HappyEyeballs LANGUAGES CXX C)

#Set Build Config Post fixes to be added to the end of generated libraries and executables
set(CMAKE_DEBUG_POSTFIX d)
#set(CMAKE_RELEASE_POSTFIX R)

#Create a /bin directory at top level of CoreTransport Dir (ie the CoreTransport Workspace Directory)
file(MAKE_DIRECTORY ./bin)
file(MAKE_DIRECTORY ./include)
file(MAKE_DIRECTORY ./include/dill)
file(MAKE_DIRECTORY ./include/CoreTransport)
file(MAKE_DIRECTORY ./include/CoreTransport/CAuth)
#file(MAKE_DIRECTORY ./include/CoreTransport/protobuf)
file(MAKE_DIRECTORY ./include/CoreTransport/CTransport)
#file(MAKE_DIRECTORY ./include/CoreTransport/CXTransport)
#file(MAKE_DIRECTORY ./include/CoreTransport/CXURL)
#file(MAKE_DIRECTORY ./include/CoreTransport/CXReQL)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Debug build.")
  add_definitions(-D_DEBUG)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
  message("Release build.")
ELSE()
  message("Some other build type.")
ENDIF()

#recurse into dill subdirectory to build
add_subdirectory(dill)

#copy libdill project binaries to CoreTransport bin

#Set CoreTransport archive bin
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#start CoreTransport project lib builds
project(CoreTransport LANGUAGES CXX C)

#recurse into CAuth subdirectory to build
add_subdirectory (CAuth)

#recurse into CTransport directory to build
add_subdirectory (CTransport)
#add_subdirectory (CXTransport)
add_subdirectory (Samples/HappyEyeballs)

if ( CTRANSPORT_WOLFSSL )
target_include_directories (CTransport PUBLIC /usr/local/include)
target_include_directories (CTransport PUBLIC /home/jmoulton/3rdGen/3rdParty/wolfssl-5.1.1/wolfssl)
target_link_libraries (CTransport PUBLIC /home/jmoulton/3rdGen/3rdParty/wolfssl-5.1.1/build/libwolfssld.a)
target_link_libraries (CTransport PUBLIC /usr/lib/libm.a)
target_include_directories (HappyEyeballs PUBLIC /usr/local/include)
target_include_directories (HappyEyeballs PUBLIC /home/jmoulton/3rdGen/3rdParty/wolfssl-5.1.1/wolfssl)
target_link_libraries (HappyEyeballs PUBLIC /home/jmoulton/3rdGen/3rdParty/wolfssl-5.1.1/build/libwolfssld.a)
#target_link_libraries (HappyEyeballs PUBLIC /usr/local/lib/libwolfssl.a)
target_link_libraries (HappyEyeballs PUBLIC /usr/lib/libm.a)
endif()

