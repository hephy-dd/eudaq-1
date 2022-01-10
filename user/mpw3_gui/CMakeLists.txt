option(USER_mpw3_gui_BUILD "build mpw3_gui" OFF)

if(NOT USER_mpw3_gui_BUILD)
  message(STATUS "mpw3_gui is NOT to be built (USER_mpw3_gui_BUILD=OFF)")
  return()
else()
  message(STATUS "mpw3_gui is to be built (USER_mpw3_gui_BUILD=ON)")
endif()

cmake_minimum_required(VERSION 3.5)

project(MPW3_gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(PROJECT_SOURCES
        defaults.h
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        monitorview.h
        monitorview.cpp
        monitorview.ui
        configcreator.h
        configcreator.cpp
        configcreator.ui
        mpw3_monitor.h
        mpw3_monitor.cc
        mpw3_qt_if.h
        mpw3_qt_if.cpp
        controller.h
        controller.cpp
        controller.ui
)

option(MPW3_MONITOR "add monitor support" ON)

if(MPW3_MONITOR)
  message(STATUS "MPW3-monitor is to be built (MPW3_MONITOR=ON)")
  #find_package(eudaq REQUIRED)
  add_compile_definitions(BUILD_MONITOR)
  #include_directories(${EUDAQ_INCLUDE_DIRS})
else()
  message(STATUS "EUDAQ-monitor is not to be built (MPW3_MONITOR=OFF)")
  list(REMOVE_ITEM PROJECT_SOURCES
      mpw3_monitor.h
      mpw3_monitor.cc
      monitorview.h
      monitorview.cpp
      monitorview.ui
      mpw3_qt_if.cpp
      mpw3_qt_if.h)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MPW3_gui
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET MPW3_gui APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(MPW3_gui SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(MPW3_gui
            ${PROJECT_SOURCES}
        )
    endif()
endif()

if(MPW3_MONITOR)
target_link_libraries(MPW3_gui ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} Qt${QT_VERSION_MAJOR}::Widgets)
else()
target_link_libraries(MPW3_gui Qt${QT_VERSION_MAJOR}::Widgets)
endif()

set_target_properties(MPW3_gui PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MPW3_gui)
endif()

install(TARGETS MPW3_gui
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
