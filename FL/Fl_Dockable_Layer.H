//
// Floating layer header file for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#ifndef Fl_Dockable_Layer_H
#define Fl_Dockable_Layer_H

#include <FL/Fl_Dockable_Export.H>
#include <FL/Fl_Group.H>

/**
  The group panels float captive in: over an Fl_Dockable_Area, in the same
  window, holding only the panels' windows, which keep their sizes as it is
  resized and stay inside it.  Events elsewhere in it go to what is below.
  \see Fl_Dockable_Area::captive_layer()
*/
class FL_DOCKABLE_EXPORT Fl_Dockable_Layer : public Fl_Group {
public:
  Fl_Dockable_Layer(int X, int Y, int W, int H);
  void resize(int X, int Y, int W, int H) override;
  int handle(int event) override;
};

#endif // !Fl_Dockable_Layer_H
