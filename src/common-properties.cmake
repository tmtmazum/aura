set(AURA_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/..)
message("<!-- Your output directory is configured as ${AURA_OUTPUT_DIR} -->")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${AURA_OUTPUT_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${AURA_OUTPUT_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${AURA_OUTPUT_DIR}/bin)

if (MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
	set(CMAKE_CXX_FLAGS_RELEASE "/MT")
endif()

include_directories(.)
include_directories(../include)
