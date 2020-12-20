project(ogl-test)

file(GLOB ogl_test_src *.cpp *.h)

add_executable(ogl-test ${ogl_test_src})
target_link_libraries(ogl-test aura_core cinder)
target_include_directories(ogl-test PRIVATE ${CINDER_INCLUDE_DIR})
if (MSVC)
    target_link_options(ogl-test PRIVATE "/SUBSYSTEM:WINDOWS")
endif()
SET_AURA_BINARY_PROPERTIES(ogl-test)
