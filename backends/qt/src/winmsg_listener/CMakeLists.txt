cmake_minimum_required(VERSION 3.10)
project(qt_srv LANGUAGES CXX)

# Make Qt's moc run automatically for files with Q_OBJECT
set(CMAKE_AUTOMOC ON)

# Qt5/Qt6 compatible find_package
find_package(QT NAMES Qt5 COMPONENTS
    Core
    Gui
    Widgets
REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS
    Core
    Gui
    Widgets 
REQUIRED)

add_library(qt_srv SHARED
    dllmain.cpp
    pipe_server.h
    pipe_server.cpp
    qt_discovery_actions.h
    qt_discovery_actions.cpp
    qt_element_summary.h
    qt_element_summary.cpp
    qt_object_store.h
    qt_object_store.cpp
    qt_property_actions.h
    qt_property_actions.cpp
    qt_replies.h
    qt_replies.cpp
    qt_server.h
    qt_server.cpp
    qt_utils.h
    qt_utils.cpp
    qt_widget_actions.h
    qt_widget_actions.cpp
)

target_link_libraries(qt_srv PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional but nice: stable DLL name
set_target_properties(qt_srv PROPERTIES OUTPUT_NAME "qt_srv")
