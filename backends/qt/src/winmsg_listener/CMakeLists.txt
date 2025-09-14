cmake_minimum_required(VERSION 3.30)
project(qt_srv LANGUAGES CXX)

# Make Qt's moc run automatically for files with Q_OBJECT
set(CMAKE_AUTOMOC ON)

# Qt5/Qt6 compatible find_package
find_package(QT NAMES Qt5 COMPONENTS
    Core
    Network
    Gui
    Widgets
REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS
    Core
    Network
    Gui
    Widgets 
REQUIRED)

add_library(qt_srv SHARED
    dllmain.cpp
    qt_server.h
    qt_server.cpp
)

target_link_libraries(qt_srv PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional but nice: stable DLL name
set_target_properties(qt_srv PROPERTIES OUTPUT_NAME "qt_srv")
