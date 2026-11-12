//
// Export macro for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#ifndef Fl_Dockable_Export_H
#define Fl_Dockable_Export_H

/*
  FL_DOCKABLE_EXPORT marks the classes of the Fl_Dockable_Area library, which is built
  apart from FLTK and so cannot use FL_EXPORT: when an application links
  FLTK's shared library, FL_EXPORT declares classes imported from FLTK's
  DLL.  Built as a static library, the default, FL_DOCKABLE_EXPORT is empty.
  Built as a shared one, FL_DOCKABLE_DLL is defined for the library and for the
  code that uses it, and FL_DOCKABLE_LIBRARY for the library alone.
*/
#if defined(FL_DOCKABLE_DLL)
#  if defined(_WIN32)
#    if defined(FL_DOCKABLE_LIBRARY)
#      define FL_DOCKABLE_EXPORT __declspec(dllexport)
#    else
#      define FL_DOCKABLE_EXPORT __declspec(dllimport)
#    endif
#  elif defined(__GNUC__) && __GNUC__ >= 4
#    define FL_DOCKABLE_EXPORT __attribute__((visibility("default")))
#  else
#    define FL_DOCKABLE_EXPORT
#  endif
#else
#  define FL_DOCKABLE_EXPORT
#endif

#endif // !Fl_Dockable_Export_H
