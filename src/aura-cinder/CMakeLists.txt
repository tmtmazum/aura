project(aura-cinder)

file(GLOB aura_cinder_src *.cpp *.h)

add_executable(aura_cinder ${aura_cinder_src})
target_link_libraries(aura_cinder aura_core cinder)
target_include_directories(aura_cinder PRIVATE ${CINDER_INCLUDE_DIR})
if (MSVC)
    target_link_options(aura_cinder PRIVATE "/SUBSYSTEM:WINDOWS")
endif()
