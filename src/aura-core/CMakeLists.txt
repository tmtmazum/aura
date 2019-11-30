project(aura-core)

file(GLOB aura_core_src *.h *.cpp)

add_library(aura_core STATIC ${aura_core_src})
target_compile_features(aura_core PUBLIC cxx_std_17)
