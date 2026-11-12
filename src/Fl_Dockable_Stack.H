//
// Stack header file for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#ifndef Fl_Dockable_Stack_H
#define Fl_Dockable_Stack_H

#include <FL/Fl_Dockable_Export.H>
#include <FL/Fl_Tile.H>

class Fl_Dockable_Area;
class Fl_Dockable_Group;

/**
  Panels stacked in one place of an Fl_Dockable_Area: one above another in a
  column, side by side in a row.  Each part is a panel, an Fl_Dockable_Tabs,
  or, in a stack along its place, a stack across the place.
  The borders between them can be dragged.  The area makes and deletes these
  as panels join and leave a place.
*/
class FL_DOCKABLE_EXPORT Fl_Dockable_Stack : public Fl_Tile {
public:
  Fl_Dockable_Stack(Fl_Dockable_Area *area, bool vertical, int X, int Y, int W, int H);
  ~Fl_Dockable_Stack() override;

  int handle(int event) override;
  void resize(int X, int Y, int W, int H) override;

  int parts();
  Fl_Widget *part(int i);
  void insert_part(Fl_Widget *part, int i);
  void layout_parts();
  /** True for a column, false for a row. */
  bool is_vertical() const { return vertical_; }
  /** 0 for a stack along its place, 1 for one across a part of that stack. */
  int level() const { return level_; }
  /** Sets the stack's level. */
  void level(int l) { level_ = l; }

private:
  static bool inside_cb(Fl_Widget *target);
  static void payload_cb(Fl_Widget *target, Fl_Dockable_Group *dockable);
  static void undock_cb(Fl_Widget *target, Fl_Dockable_Group *dockable);

  Fl_Dockable_Area *area_;
  bool vertical_;
  int level_;
};

#endif // !Fl_Dockable_Stack_H
