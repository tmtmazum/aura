project(aura-game)

file(GLOB aura_cinder_src cinder/*.cpp cinder/*.h)
file(GLOB aura_game_src *.cpp *.h)

add_executable(aura-game ${aura_game_src} ${aura_cinder_src})
target_link_libraries(aura-game aura_core cinder)
target_include_directories(aura-game PRIVATE ${CINDER_INCLUDE_DIR})
if (MSVC)
    target_link_options(aura-game PRIVATE "/SUBSYSTEM:WINDOWS")
endif()
source_group("cinder" FILES ${aura_cinder_src})

