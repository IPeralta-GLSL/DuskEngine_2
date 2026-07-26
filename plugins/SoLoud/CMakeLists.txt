o3de_gem_setup("SoLoud")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})

find_package(SoLoudLib REQUIRED)

add_subdirectory(Code)
