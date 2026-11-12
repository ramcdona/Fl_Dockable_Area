//
// Floating layer for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#include <FL/Fl_Dockable_Layer.H>
#include <FL/Fl.H>

/**
  Creates an empty layer at X, Y, W, H, which should be the area's own
  rectangle, and adds it to the current group.  It does not become the
  current group itself.
*/
Fl_Dockable_Layer::Fl_Dockable_Layer(int X, int Y, int W, int H) : Fl_Group(X, Y, W, H) {
  // Ended at once: the group made current before it is current again
  end();
  box(FL_NO_BOX);
}

/**
  Gives the mouse to a window floating in the layer, or else to what is below
  it: the center and the docked panels, whose borders show the resize cursor.
*/
int Fl_Dockable_Layer::handle(int event) {
  if (event == FL_ENTER || event == FL_MOVE || event == FL_PUSH || event == FL_DND_ENTER ||
      event == FL_DND_DRAG || event == FL_MOUSEWHEEL) {
    for (int i = 0; i < children(); i++) {
      Fl_Widget *c = child(i);
      if (c->visible() && Fl::event_inside(c)) {
        return Fl_Group::handle(event);
      }
    }
    return 0;
  }
  return Fl_Group::handle(event);
}

/**
  Moves the floating windows with the layer, keeping their sizes and keeping
  them inside it as far as they fit.
*/
void Fl_Dockable_Layer::resize(int X, int Y, int W, int H) {
  int dx = X - x();
  int dy = Y - y();
  Fl_Widget::resize(X, Y, W, H);
  for (int i = 0; i < children(); i++) {
    Fl_Widget *c = child(i);
    int cx = c->x() + dx;
    int cy = c->y() + dy;
    if (cx + c->w() > X + W) {
      cx = X + W - c->w();
    }
    if (cy + c->h() > Y + H) {
      cy = Y + H - c->h();
    }
    if (cx < X) {
      cx = X;
    }
    if (cy < Y) {
      cy = Y;
    }
    c->position(cx, cy);
  }
}
