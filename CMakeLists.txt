cmake_minimum_required(VERSION 3.21)

project(GarageDoor LANGUAGES C)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/dependencies")
find_package(cppmodel REQUIRED)

enable_testing()

add_executable(SimulateGarageDoor garageDoor.c SimulateGarageDoor.c)
target_link_libraries(SimulateGarageDoor PRIVATE cppmodel::cppmodel)
cppmodel_deploy_runtime(SimulateGarageDoor)

add_test(NAME GarageDoor COMMAND SimulateGarageDoor)
