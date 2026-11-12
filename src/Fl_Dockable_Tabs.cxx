//
// Tabs for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#include "Fl_Dockable_Tabs.H"
#include <FL/Fl_Dockable_Panel.H>
#include <FL/Fl.H>

Fl_Dockable_Tabs::Fl_Dockable_Tabs(Fl_Dockable_Area *area, int X, int Y, int W, int H)
    : Fl_Tabs(X, Y, W, H) {
  // Ended at once: the group made current before it is current again
  end();
  area_ = area;
  pressed_ = nullptr;
}

Fl_Dockable_Tabs::~Fl_Dockable_Tabs() {
  Fl_Dockable_Group::check_origin_target(this);
}

int Fl_Dockable_Tabs::panels() {
  int n = 0;
  for (int i = 0; i < children(); i++) {
    if (dynamic_cast<Fl_Dockable_Panel *>(child(i))) {
      n++;
    }
  }
  return n;
}

Fl_Dockable_Panel *Fl_Dockable_Tabs::panel(int i) {
  for (int k = 0; k < children(); k++) {
    Fl_Dockable_Panel *d = dynamic_cast<Fl_Dockable_Panel *>(child(k));
    if (d && i-- == 0) {
      return d;
    }
  }
  return nullptr;
}

void Fl_Dockable_Tabs::add_panel(Fl_Dockable_Panel *d) {
  int X, Y, W, H;
  client_area(X, Y, W, H);
  d->show_bar(false);
  add(d);
  d->resize(X, Y, W, H);
  init_sizes();
  value(d);
}

// A panel's tab is its title bar: pressing it selects the panel, and dragging it drags the panel
// out of the tabs. The right mouse button on a tab opens its panel's menu. Only a panel dragged
// out comes here as FL_UNDOCK; the area takes the drops.
int Fl_Dockable_Tabs::handle(int event) {
  if (event == FL_PUSH) {
    pressed_ = nullptr;
    Fl_Dockable_Panel *d = dynamic_cast<Fl_Dockable_Panel *>(which(Fl::event_x(), Fl::event_y()));
    if (d && Fl::event_button() == FL_RIGHT_MOUSE) {
      value(d);
      d->show_menu();
      return 1;
    }
    if (d && Fl::event_button() == FL_LEFT_MOUSE) {
      if (d != value()) {
        value(d);
        redraw();
      }
      // A panel that cannot both move and float is not dragged
      int drag = FL_DOCKABLE_MOVABLE | FL_DOCKABLE_FLOATABLE;
      if ((d->dock_flags() & drag) == drag) {
        pressed_ = d;
        d->handle_drag_widget(event);
      }
    }
  } else if ((event == FL_DRAG || event == FL_RELEASE) && pressed_ && find(pressed_) == children()) {
    // The panel pressed has left the tabs
    pressed_ = 0;
  } else if ((event == FL_DRAG || event == FL_RELEASE) && pressed_) {
    Fl_Dockable_Panel *d = pressed_;
    if (event == FL_RELEASE) {
      pressed_ = nullptr;
    }
    int r = d->handle_drag_widget(event);
    if (event == FL_RELEASE) {
      d->released();
    }
    if (r) {
      return r;
    }
  }
  if (event == FL_UNDOCK) {
    bool processed = false;
    int r = Fl_Dockable_Group::handle_docking_target(processed, this, event, inside_cb, nullptr,
                                                     nullptr, payload_cb, undock_cb);
    if (processed) {
      return r;
    }
  }
  return Fl_Tabs::handle(event);
}

bool Fl_Dockable_Tabs::inside_cb(Fl_Widget *target) {
  return false;
}

void Fl_Dockable_Tabs::payload_cb(Fl_Widget *target, Fl_Dockable_Group *dockable) {}

void Fl_Dockable_Tabs::undock_cb(Fl_Widget *target, Fl_Dockable_Group *dockable) {
  Fl_Dockable_Tabs *tabs = (Fl_Dockable_Tabs *)target;

  // FLTK leaves a Docking_Target_Box where the panel was; the tabs close up instead
  for (int i = tabs->children() - 1; i >= 0; i--) {
    Fl_Widget *c = tabs->child(i);
    if (!dynamic_cast<Fl_Dockable_Panel *>(c)) {
      tabs->remove(c);
      Fl::delete_widget(c);
      break;
    }
  }
  // The layout is saved when the drag ends, with the panel's window where it was left
  Fl_Dockable_Panel *d = dynamic_cast<Fl_Dockable_Panel *>(dockable);
  if (d) {
    d->show_bar(true);
  }
  tabs->area_->untabbed(tabs);
}
