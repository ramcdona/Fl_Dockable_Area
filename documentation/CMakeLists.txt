#
# CMake build of the documentation of the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
#
# Copyright 2026 by Rob McDonald and others.
#
# This library is free software. Distribution and use rights are outlined in
# the file "COPYING" which should have been included with this file. If this
# file is missing or damaged, see the license at:
#
#     https://www.fltk.org/COPYING.php
#

# The HTML manual, made with Doxygen as FLTK's is: cmake --build . --target html

set(LOGFILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile_error.log")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
  ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
  @ONLY
)

add_custom_target(html
  COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating HTML documentation" VERBATIM
)

if(FL_DOCKABLE_AREA_INSTALL_HTML_DOCS)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/Fl_Dockable_Area
  )
endif()
