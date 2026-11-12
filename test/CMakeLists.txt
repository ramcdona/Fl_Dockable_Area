#
# Test programs for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
#
# Copyright 2026 by Rob McDonald and others.
#
# This library is free software. Distribution and use rights are outlined in
# the file "COPYING" which should have been included with this file. If this
# file is missing or damaged, see the license at:
#
#     https://www.fltk.org/COPYING.php
#

# Unit tests of places, docking, tabs, stacks and layouts.  A panel shows only
# in a window that is shown, so they need a display.
add_executable(unittest_dockable unittest_dockable.cxx)
target_link_libraries(unittest_dockable PRIVATE fltk::dockable_area)
# The tests also look at the groups the area makes, whose headers are internal
target_include_directories(unittest_dockable PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME unittest_dockable COMMAND unittest_dockable)
set_tests_properties(unittest_dockable PROPERTIES LABELS display TIMEOUT 60)

# The demo: a center box and panels around it, one of them an OpenGL view.
add_executable(dockable_area WIN32 MACOSX_BUNDLE dockable_area.cxx)
target_link_libraries(dockable_area PRIVATE fltk::dockable_area fltk::gl)
