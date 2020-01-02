project(aura-client)

file(GLOB aura_client_src *.h *.cpp)

add_library(aura_client SHARED ${aura_client_src})
target_compile_features(aura_client PUBLIC cxx_std_17)
