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
#include <FL/Fl_Dockable_Panel.H>
#include <FL/Fl.H>
#include "Fl_Dockable_Private.H"

#include <algorithm>

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

enum { HOLDER_LEFT = 1, HOLDER_RIGHT = 2, HOLDER_TOP = 4, HOLDER_BOTTOM = 8 };

Fl_Dockable_Holder::Fl_Dockable_Holder(int X, int Y, int W, int H) : Fl_Double_Window(X, Y, W, H) {
  box(FL_UP_BOX);
  edge_ = 0;
  sizing_ = 0;
  push_x_ = 0;
  push_y_ = 0;
}

/**
  The frame around the panel, which sizes the window when dragged: the frame
  of the window FLTK drags a panel in, so the panel keeps its place and size
  as it moves between the two.
*/
int Fl_Dockable_Holder::margin() {
  return Fl::box_dx(FL_UP_BOX);
}

// The edges within reach of the mouse: as in the window FLTK drags a panel in, a little more than
// the frame, so the frame is not too thin to take hold of
int Fl_Dockable_Holder::edges_at(int X, int Y) const {
  int m = 2 * Fl::box_dx(FL_DOWN_BOX);
  int e = 0;
  if (X < m) {
    e |= HOLDER_LEFT;
  } else if (X >= w() - m) {
    e |= HOLDER_RIGHT;
  }
  if (Y < m) {
    e |= HOLDER_TOP;
  } else if (Y >= h() - m) {
    e |= HOLDER_BOTTOM;
  }
  return e;
}

void Fl_Dockable_Holder::set_edge_cursor(int e) {
  Fl_Cursor c = FL_CURSOR_DEFAULT;
  if (e == (HOLDER_LEFT | HOLDER_TOP) || e == (HOLDER_RIGHT | HOLDER_BOTTOM)) {
    c = FL_CURSOR_NWSE;
  } else if (e == (HOLDER_RIGHT | HOLDER_TOP) || e == (HOLDER_LEFT | HOLDER_BOTTOM)) {
    c = FL_CURSOR_NESW;
  } else if (e & (HOLDER_LEFT | HOLDER_RIGHT)) {
    c = FL_CURSOR_WE;
  } else if (e & (HOLDER_TOP | HOLDER_BOTTOM)) {
    c = FL_CURSOR_NS;
  }
  cursor(c);
}

// Nothing docks in a holder: a panel dropped over it floats captive at the point, as the area says
bool Fl_Dockable_Holder::inside_cb(Fl_Widget *) {
  return false;
}

void Fl_Dockable_Holder::payload_cb(Fl_Widget *, Fl_Dockable_Group *) {
}

// FLTK leaves a Docking_Target_Box where the panel was; the holder goes with it
void Fl_Dockable_Holder::undock_cb(Fl_Widget *target, Fl_Dockable_Group *) {
  Fl_Dockable_Holder *h = (Fl_Dockable_Holder *)target;
  h->hide();
  if (h->parent()) {
    h->parent()->remove(h);
  }
  Fl::delete_widget(h);
}

/**
  Gives FLTK the panel undocked from it, sizes the window from its frame, and
  passes the docking events of a panel dragged over it on to the area below.
*/
int Fl_Dockable_Holder::handle(int event) {
  if (event == FL_UNDOCK) {
    bool processed = false;
    int r = Fl_Dockable_Group::handle_docking_target(processed, this, event, inside_cb, nullptr, nullptr,
                                                     payload_cb, undock_cb);
    if (processed) {
      return r;
    }
  }
  if (event == FL_DOCK_ENTER || event == FL_DOCK_DRAG || event == FL_DOCK_LEAVE || event == FL_DOCK_RELEASE) {
    return 0;
  }
  switch (event) {
    case FL_ENTER:
    case FL_MOVE: {
      int e = edges_at(Fl::event_x(), Fl::event_y());
      if (e != edge_) {
        edge_ = e;
        set_edge_cursor(e);
      }
      if (e) {
        return 1;
      }
      break;
    }
    case FL_LEAVE:
      if (!sizing_ && edge_) {
        edge_ = 0;
        set_edge_cursor(0);
      }
      break;
    case FL_PUSH:
      if (edge_ && Fl::event_button() == FL_LEFT_MOUSE) {
        sizing_ = edge_;
        push_x_ = Fl::event_x_root();
        push_y_ = Fl::event_y_root();
        return 1;
      }
      break;
    case FL_DRAG:
      if (sizing_) {
        int dx = Fl::event_x_root() - push_x_;
        int dy = Fl::event_y_root() - push_y_;
        int X = x();
        int Y = y();
        int W = w();
        int H = h();
        int least = 4 * margin() + Fl_Dockable_Panel::bar_height();
        if (sizing_ & HOLDER_LEFT) {
          dx = std::min(dx, W - least);
          X += dx;
          W -= dx;
        } else if (sizing_ & HOLDER_RIGHT) {
          W = std::max(least, W + dx);
        }
        if (sizing_ & HOLDER_TOP) {
          dy = std::min(dy, H - least);
          Y += dy;
          H -= dy;
        } else if (sizing_ & HOLDER_BOTTOM) {
          H = std::max(least, H + dy);
        }
        resize(X, Y, W, H);
        push_x_ = Fl::event_x_root();
        push_y_ = Fl::event_y_root();
        return 1;
      }
      break;
    case FL_RELEASE:
      if (sizing_) {
        sizing_ = 0;
        Fl_Dockable_Panel *d = nullptr;
        if (children() > 0) {
          d = dynamic_cast<Fl_Dockable_Panel *>(child(0));
        }
        if (d) {
          d->holder_sized();
        }
        return 1;
      }
      break;
    default:
      break;
  }
  return Fl_Double_Window::handle(event);
}
