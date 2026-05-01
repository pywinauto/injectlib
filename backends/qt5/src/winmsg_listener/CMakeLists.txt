cmake_minimum_required(VERSION 3.10)
project(qt5_srv LANGUAGES CXX)

# Make Qt's moc run automatically for files with Q_OBJECT
set(CMAKE_AUTOMOC ON)

# Qt5 backend
find_package(Qt5 COMPONENTS
    Core
    Gui
    Widgets 
REQUIRED)

add_library(qt5_srv SHARED
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

target_link_libraries(qt5_srv PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional but nice: stable DLL name
set_target_properties(qt5_srv PROPERTIES OUTPUT_NAME "qt5_srv")
