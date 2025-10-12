cmake_minimum_required(VERSION 3.15)
project(injected_dotnet LANGUAGES CXX CSharp)

set(BACKEND_BIN_DIR ${CMAKE_CURRENT_LIST_DIR}/bin)

add_subdirectory(src/bootstrap)
add_subdirectory(src/wpf)