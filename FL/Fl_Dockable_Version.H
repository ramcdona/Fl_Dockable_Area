//
// Version header file for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

/** \file
  The version of the Fl_Dockable_Area library, as FLTK gives its own in
  FL/Enumerations.H.  CMake reads the version from this file.
*/

#ifndef Fl_Dockable_Version_H
#define Fl_Dockable_Version_H

/** The major version: changes that break code built for an earlier one. */
#define FL_DOCKABLE_AREA_MAJOR_VERSION 0

/** The minor version: features added. */
#define FL_DOCKABLE_AREA_MINOR_VERSION 1

/** The patch version: fixes only. */
#define FL_DOCKABLE_AREA_PATCH_VERSION 0

/**
  The version as one number: 10000 * major + 100 * minor + patch, as FLTK's
  FL_API_VERSION.  Compare it with Fl_Dockable_Area::api_version(), the
  version of the library linked, to find a header and library that differ.
*/
#define FL_DOCKABLE_AREA_API_VERSION                                                               \
  (FL_DOCKABLE_AREA_MAJOR_VERSION * 10000 + FL_DOCKABLE_AREA_MINOR_VERSION * 100 +                 \
   FL_DOCKABLE_AREA_PATCH_VERSION)

#endif // !Fl_Dockable_Version_H
