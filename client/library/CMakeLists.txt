cmake_minimum_required(VERSION 3.15)

# Project details
project(
	"battleship-client-library"
	VERSION 0.1.0 
	LANGUAGES CXX
)

# Set project options
include(cmake/StandardSettings.cmake)
include(cmake/Utils.cmake)
message(STATUS "Started CMake for ${PROJECT_NAME} v${PROJECT_VERSION}...\n")

# Setup alternative names
if(${PROJECT_NAME}_USE_ALT_NAMES)
	string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
	string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)
else()
	set(PROJECT_NAME_LOWERCASE ${PROJECT_NAME})
	set(PROJECT_NAME_UPPERCASE ${PROJECT_NAME})
endif()

# Prevent building in the source directory
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.\n")
endif()

# Create library, setup header and source files
# Find all headers and implementation files
include(cmake/SourcesAndHeaders.cmake)

add_library(
	${PROJECT_NAME}
	${headers}
	${sources}
)

verbose_message("Found the following header files:")
verbose_message(${headers})
verbose_message("Found the following source files:")
verbose_message(${sources})
message(STATUS "Added all header and implementation files.\n")

# Set standard & warnings
set(CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(${PROJECT_NAME})
verbose_message("Applied compiler warnings. Using standard ${CXX_STANDARD}.\n")

# Set the build/user include directories
# Allow usage of header files in the `src` directory, but only for utilities
target_include_directories(
	${PROJECT_NAME}
	PUBLIC 
		$<INSTALL_INTERFACE:include>		
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

message(STATUS "Finished setting up include directories.")

target_link_libraries(${PROJECT_NAME} battleship-common)
verbose_message("Successfully added all dependencies and linked against them.")

# Add version header
configure_file(
	${CMAKE_CURRENT_LIST_DIR}/cmake/version.hpp.in
	include/${PROJECT_NAME_LOWERCASE}/version.hpp
	@ONLY
)

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME_LOWERCASE}/version.hpp
	DESTINATION include/${PROJECT_NAME_LOWERCASE}
)

# Install the `include` directory
install(DIRECTORY include/${PROJECT_NAME_LOWERCASE} DESTINATION include)

verbose_message("Install targets succesfully build. Install with `cmake --build <build_directory> --target install --config <build_config>`.")

# Quick `ConfigVersion.cmake` creation
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	${PROJECT_NAME}ConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
	${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Generate export header if specified
if(${PROJECT_NAME}_GENERATE_EXPORT_HEADER)
	include(GenerateExportHeader)
	generate_export_header(${PROJECT_NAME})
	install(
		FILES
			${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWERCASE}_export.h 
		DESTINATION
			include
	)

message(STATUS "Generated the export header `${PROJECT_NAME_LOWERCASE}_export.h` and installed it.")
endif()

message(STATUS "Finished building requirements for installing the package.\n")

# Unit testing setup
if(${PROJECT_NAME}_ENABLE_UNIT_TESTING)
	enable_testing()
	message(STATUS "Build unit tests for the project. Tests should always be found in the test folder\n")
	add_subdirectory(test)
endif()
