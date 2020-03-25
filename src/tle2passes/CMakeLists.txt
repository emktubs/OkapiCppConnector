set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
find_package(Nova REQUIRED)
include_directories(${NOVA_INCLUDE_DIR})

add_executable(tle2passes tle2passes.cpp)
target_link_libraries(tle2passes okapi-connector ${NOVA_LIBRARIES})

# Install the libraries into the lib directory and executrables to bin
install(TARGETS tle2passes
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
