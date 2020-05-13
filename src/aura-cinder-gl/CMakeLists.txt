project(aura-cinder-gl)

file(GLOB aura_cinder_gl_src *.cpp *.h)

add_executable(aura_cinder_gl ${aura_cinder_gl_src})
target_link_libraries(aura_cinder_gl aura_core cinder)
target_include_directories(aura_cinder_gl PRIVATE ${CINDER_INCLUDE_DIR})
if (MSVC)
    target_link_options(aura_cinder_gl PRIVATE "/SUBSYSTEM:WINDOWS")
endif()
