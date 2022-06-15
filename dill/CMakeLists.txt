# CMake build system is not officially supported.
# If you find out it's not working fix it and send a pull request.

cmake_minimum_required(VERSION 3.1)
project(libdill VERSION 1.6 LANGUAGES C)

include(CheckSymbolExists)
include(CheckFunctionExists)

file(GLOB sources ${CMAKE_CURRENT_LIST_DIR}/*.c ${CMAKE_CURRENT_LIST_DIR}/dns/dns.c)
include_directories(${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/dns")
set_source_files_properties(dns/dns.c PROPERTIES COMPILE_FLAGS -std=c99)
add_library(dill ${sources})

# add pthread
list(APPEND CMAKE_REQUIRED_LIBRARIES pthread)
target_link_libraries(dill pthread)

# add openssl # requires libssl-dev on ubuntu
list(APPEND CMAKE_REQUIRED_LIBRARIES ssl crypto)
target_link_libraries(dill ssl crypto)

# check and enable rt if available
list(APPEND CMAKE_REQUIRED_LIBRARIES rt)
check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)
if(HAVE_CLOCK_GETTIME)
  target_link_libraries(dill rt)
endif()

# Installation (https://github.com/forexample/package-example)

# Layout. This works for all platforms:
#   * <prefix>/lib/cmake/<PROJECT-NAME>
#   * <prefix>/lib/
#   * <prefix>/include/
set(config_install_dir "lib/cmake/${PROJECT_NAME}")
set(include_install_dir "include")

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Configuration
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

# Include module with fuction 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
# Use:
#   * PROJECT_VERSION
write_basic_package_version_file(
    "${version_config}" COMPATIBILITY SameMajorVersion
)

# Configure '<PROJECT-NAME>Config.cmake'
# Use variables:
#   * TARGETS_EXPORT_NAME
#   * PROJECT_NAME
configure_package_config_file(
    "cmake/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

#will ensure libdill build ouptut gets copied to CoreTransport/bin
set_target_properties(dill PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(dill PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Targets:
#   * <prefix>/lib/libdill.a
#   * header location after install: <prefix>/include/libdill.h
install(
    TARGETS dill
    EXPORT "${TARGETS_EXPORT_NAME}"
    INCLUDES DESTINATION "${include_install_dir}"
#    LIBRARY DESTINATION "${CMAKE_BINARY_DIR}/bin"
#    ARCHIVE DESTINATION "${CMAKE_BINARY_DIR}/bin"
#    RUNTIME DESTINATION "${CMAKE_BINARY_DIR}/bin"
)

# Headers:
#   * libdill.h -> <prefix>/include/libdill.h
install(
    FILES libdill.h
    DESTINATION "${include_install_dir}"
)

# Config
#   * <prefix>/lib/cmake/libdill/libdillConfig.cmake
#   * <prefix>/lib/cmake/libdill/libdillConfigVersion.cmake
install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

# Config
#   * <prefix>/lib/cmake/libdill/libdillTargets.cmake
install(
    EXPORT "${TARGETS_EXPORT_NAME}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
)

set(CMAKE_REQUIRED_LIBRARIES )

# check and enable stack guard and dns if available
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)

set(CMAKE_REQUIRED_LIBRARIES )
set(CMAKE_REQUIRED_DEFINITIONS )

add_definitions(-DDILL_THREADS)

check_function_exists(mprotect HAVE_MPROTECT)
if(HAVE_MPROTECT)
  add_definitions(-DHAVE_MPROTECT)
endif()

check_function_exists(posix_memalign HAVE_POSIX_MEMALIGN)
if(HAVE_POSIX_MEMALIGN)
  add_definitions(-DHAVE_POSIX_MEMALIGN)
endif()

#Add a post build step that will copy include files to directory structure that should be utilized by projects that link CTransport
add_custom_command(TARGET dill POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/dill
    COMMENT "Copying slimdill Source Headers to dill/include/ directory."
)

