//
// Tabs header file for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#ifndef Fl_Dockable_Tabs_H
#define Fl_Dockable_Tabs_H

#include <FL/Fl_Dockable_Export.H>
#include <FL/Fl_Tabs.H>

class Fl_Dockable_Area;
class Fl_Dockable_Panel;
class Fl_Dockable_Group;

/**
  Panels docked in one place of an Fl_Dockable_Area, as tabs.  A panel's tab
  is its title bar: pressing it selects the panel and dragging it drags the
  panel out of the tabs.  The area makes and deletes these as panels join and
  leave a place.
*/
class FL_DOCKABLE_EXPORT Fl_Dockable_Tabs : public Fl_Tabs {
public:
  Fl_Dockable_Tabs(Fl_Dockable_Area *area, int X, int Y, int W, int H);
  ~Fl_Dockable_Tabs() override;

  int handle(int event) override;

  void add_panel(Fl_Dockable_Panel *d);
  int panels();
  Fl_Dockable_Panel *panel(int i);

private:
  static bool inside_cb(Fl_Widget *target);
  static void payload_cb(Fl_Widget *target, Fl_Dockable_Group *dockable);
  static void undock_cb(Fl_Widget *target, Fl_Dockable_Group *dockable);

  Fl_Dockable_Area *area_;
  // The panel whose tab was pressed, which a drag drags
  Fl_Dockable_Panel *pressed_;
};

#endif // !Fl_Dockable_Tabs_H
