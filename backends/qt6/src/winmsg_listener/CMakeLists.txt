cmake_minimum_required(VERSION 3.10)
project(qt6_srv LANGUAGES CXX)

# Make Qt's moc run automatically for files with Q_OBJECT
set(CMAKE_AUTOMOC ON)

# Qt6 backend
find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets 
REQUIRED)

set(QT6_SRV_PLATFORM_SOURCES)
if(WIN32)
    list(APPEND QT6_SRV_PLATFORM_SOURCES dllmain.cpp)
else()
    list(APPEND QT6_SRV_PLATFORM_SOURCES linux_entry.cpp)
endif()

add_library(qt6_srv SHARED
    ${QT6_SRV_PLATFORM_SOURCES}
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

target_link_libraries(qt6_srv PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# stable DLL name
set_target_properties(qt6_srv PROPERTIES OUTPUT_NAME "qt6_srv" PREFIX "")
