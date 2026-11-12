//
// Stripe header file for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#ifndef Fl_Dockable_Stripe_H
#define Fl_Dockable_Stripe_H

#include <FL/Fl_Dockable_Export.H>
#include <FL/Fl_Widget.H>

#include <vector>

class Fl_Dockable_Area;
class Fl_Dockable_Panel;

/**
  A stripe along a border of an Fl_Dockable_Area: a button for each place
  given one, which shows its panel, docked, floating or sliding out over the
  center if it auto-hides, or hides it.  A button is drawn pressed while its
  panel is shown.  Its menu, on the right mouse button, auto-hides or pins the
  panel, moves the button to another stripe or removes it; dragging it moves
  it along its stripe or to another, or drops the panel into the area.  An
  empty stripe takes no room but while a button is dragged.
*/
class FL_DOCKABLE_EXPORT Fl_Dockable_Stripe : public Fl_Widget {
public:
  Fl_Dockable_Stripe(Fl_Dockable_Area *area, int side);

  int handle(int event) override;
  void draw() override;

  Fl_Dockable_Panel *button_at(int X, int Y);
  /** Forgets a panel being deleted, if its button is pressed. */
  void forget(Fl_Dockable_Panel *d) {
    if (pressed_ == d) {
      pressed_ = 0;
      dragging_ = false;
    }
  }
  double order_at(int X, int Y);

private:
  void button_box(std::vector<Fl_Dockable_Panel *> &panels, int i, int &X, int &Y, int &W, int &H);
  void show_menu(Fl_Dockable_Panel *d);

  Fl_Dockable_Area *area_;
  int side_;
  Fl_Dockable_Panel *pressed_;
  int press_x_;
  int press_y_;
  bool dragging_;
};

#endif // !Fl_Dockable_Stripe_H
