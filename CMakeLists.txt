#
# CMake build for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
#
# Copyright 2026 by Rob McDonald and others.
#
# This library is free software. Distribution and use rights are outlined in
# the file "COPYING" which should have been included with this file. If this
# file is missing or damaged, see the license at:
#
#     https://www.fltk.org/COPYING.php
#

cmake_minimum_required(VERSION 3.15)

# The version is FL/Fl_Dockable_Version.H's.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/FL/Fl_Dockable_Version.H FL_DOCKABLE_VERSION_LINES
  REGEX "^#define FL_DOCKABLE_AREA_(MAJOR|MINOR|PATCH)_VERSION ")
foreach(part MAJOR MINOR PATCH)
  string(REGEX REPLACE ".*FL_DOCKABLE_AREA_${part}_VERSION ([0-9]+).*" "\\1" FL_DOCKABLE_${part}
    "${FL_DOCKABLE_VERSION_LINES}")
endforeach()
project(Fl_Dockable_Area VERSION ${FL_DOCKABLE_MAJOR}.${FL_DOCKABLE_MINOR}.${FL_DOCKABLE_PATCH}
  LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FLTK 1.5 or later, with Fl_Dockable_Group.  FLTK's package accepts only a
# request for its own minor version, so the lowest one is checked here.
find_package(FLTK CONFIG REQUIRED)
if(FLTK_VERSION VERSION_LESS 1.5)
  message(FATAL_ERROR "Fl_Dockable_Area needs FLTK 1.5 or later, found ${FLTK_VERSION}")
endif()
get_target_property(FL_DOCKABLE_FLTK_INCLUDES fltk::fltk INTERFACE_INCLUDE_DIRECTORIES)
find_file(FL_DOCKABLE_GROUP_HEADER FL/Fl_Dockable_Group.H PATHS ${FL_DOCKABLE_FLTK_INCLUDES}
  NO_DEFAULT_PATH)
if(NOT FL_DOCKABLE_GROUP_HEADER)
  message(FATAL_ERROR "Fl_Dockable_Area needs an FLTK with Fl_Dockable_Group")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# The headers installed for applications
set(FL_DOCKABLE_HEADERS
  FL/Fl_Dockable_Area.H
  FL/Fl_Dockable_Export.H
  FL/Fl_Dockable_Layer.H
  FL/Fl_Dockable_Panel.H
  FL/Fl_Dockable_Version.H
)

# Static unless BUILD_SHARED_LIBS is set.
add_library(fltk_dockable_area
  src/Fl_Dockable_Area.cxx
  src/Fl_Dockable_Bar.cxx
  src/Fl_Dockable_Layer.cxx
  src/Fl_Dockable_Layout.cxx
  src/Fl_Dockable_Panel.cxx
  src/Fl_Dockable_Stack.cxx
  src/Fl_Dockable_Stripe.cxx
  src/Fl_Dockable_Tabs.cxx
  src/Fl_Dockable_Bar.H
  src/Fl_Dockable_Private.H
  src/Fl_Dockable_Stack.H
  src/Fl_Dockable_Stripe.H
  src/Fl_Dockable_Tabs.H
  ${FL_DOCKABLE_HEADERS}
)
add_library(fltk::dockable_area ALIAS fltk_dockable_area)
target_include_directories(fltk_dockable_area PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(fltk_dockable_area PUBLIC fltk::fltk)
set_target_properties(fltk_dockable_area PROPERTIES EXPORT_NAME dockable_area)
if(BUILD_SHARED_LIBS)
  target_compile_definitions(fltk_dockable_area PUBLIC FL_DOCKABLE_DLL PRIVATE FL_DOCKABLE_LIBRARY)
  set_target_properties(fltk_dockable_area PROPERTIES CXX_VISIBILITY_PRESET hidden)
endif()
if(NOT MSVC)
  target_compile_options(fltk_dockable_area PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

install(TARGETS fltk_dockable_area EXPORT Fl_Dockable_AreaTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${FL_DOCKABLE_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/FL)
install(EXPORT Fl_Dockable_AreaTargets NAMESPACE fltk::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fl_Dockable_Area)
# The build tree holds a package too, so find_package() works from either.
export(EXPORT Fl_Dockable_AreaTargets NAMESPACE fltk::
  FILE ${CMAKE_CURRENT_BINARY_DIR}/Fl_Dockable_AreaTargets.cmake)

configure_package_config_file(cmake/Fl_Dockable_AreaConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/Fl_Dockable_AreaConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fl_Dockable_Area
)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/Fl_Dockable_AreaConfigVersion.cmake
  COMPATIBILITY SameMinorVersion
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/Fl_Dockable_AreaConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/Fl_Dockable_AreaConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fl_Dockable_Area
)
