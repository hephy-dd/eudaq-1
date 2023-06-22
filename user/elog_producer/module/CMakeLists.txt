cmake_minimum_required(VERSION 3.1.0)
#include(CMakeDependentOption)
find_package(Qt5Widgets REQUIRED)
message(STATUS "Qt5Widgets: ${Qt5Widgets_DIR}")
set(EXTERNAL_LIB_QT Qt5::Widgets)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(include)
#aux_source_directory(src MODULE_SRC)

qt5_wrap_ui(elogProd_UISrcs ui/elogproducer.ui)
qt5_wrap_cpp(elogProd_MOCSrcs include/elogproducer.h include/elog.h include/producer2guiproxy.h)

set(ELOG_PROD_GUI_SRC
  src/elogproducer.cpp
  src/producer2guiproxy.cpp
  src/elog.cc
  src/euElog.cxx
  )

list(APPEND ELOG_PROD_GUI_SRC ${elogProd_UISrcs} ${elogProd_MOCSrcs})

add_executable(euElog ${ELOG_PROD_GUI_SRC} )
set_property(TARGET euElog PROPERTY PROJECT_LABEL "BinEuElog")
target_link_libraries(euElog ${EUDAQ_CORE_LIBRARY} ${EXTERNAL_LIB_QT})

install(TARGETS euElog
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
