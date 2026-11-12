//
// Title bar header file for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#ifndef Fl_Dockable_Bar_H
#define Fl_Dockable_Bar_H

#include <FL/Fl_Dockable_Export.H>
#include <FL/Fl_Box.H>

class Fl_Dockable_Panel;

/**
  The title bar of an Fl_Dockable_Panel: the panel's title, a menu button and
  a close button.  Dragging it drags the panel out of the area, and back; the
  right mouse button anywhere on it opens the panel's menu.
*/
class FL_DOCKABLE_EXPORT Fl_Dockable_Bar : public Fl_Box {
public:
  Fl_Dockable_Bar(Fl_Dockable_Panel *panel, int X, int Y, int W, int H);

  int handle(int event) override;
  void draw() override;

private:
  void close_box(int &bx, int &by, int &bs);
  void menu_box(int &bx, int &by, int &bs);

  Fl_Dockable_Panel *panel_;
};

#endif // !Fl_Dockable_Bar_H
