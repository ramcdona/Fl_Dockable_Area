//
// Stack for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#include "Fl_Dockable_Stack.H"
#include "Fl_Dockable_Tabs.H"
#include <FL/Fl_Dockable_Panel.H>
#include "Fl_Dockable_Private.H"
#include <FL/Fl.H>

#include <vector>

Fl_Dockable_Stack::Fl_Dockable_Stack(Fl_Dockable_Area *area, bool vertical, int X, int Y, int W,
                                     int H)
    : Fl_Tile(X, Y, W, H) {
  // Ended at once: the group made current before it is current again
  end();
  area_ = area;
  vertical_ = vertical;
  level_ = 0;
}

Fl_Dockable_Stack::~Fl_Dockable_Stack() {
  Fl_Dockable_Group::check_origin_target(this);
}

int Fl_Dockable_Stack::parts() {
  int n = 0;
  for (int i = 0; i < children(); i++) {
    if (fl_dockable_is_part(child(i))) {
      n++;
    }
  }
  return n;
}

Fl_Widget *Fl_Dockable_Stack::part(int i) {
  for (int k = 0; k < children(); k++) {
    if (fl_dockable_is_part(child(k)) && i-- == 0) {
      return child(k);
    }
  }
  return nullptr;
}

void Fl_Dockable_Stack::insert_part(Fl_Widget *part, int i) {
  int k = 0;
  int n = 0;
  while (k < children() && n < i) {
    if (fl_dockable_is_part(child(k))) {
      n++;
    }
    k++;
  }
  insert(*part, k);
  layout_parts();
}

void Fl_Dockable_Stack::resize(int X, int Y, int W, int H) {
  Fl_Widget::resize(X, Y, W, H);
  layout_parts();
}


// Each part takes its size along the stack, a part without one the mean of the others', all scaled
// to fill it
void Fl_Dockable_Stack::layout_parts() {
  int n = parts();
  if (n == 0) {
    return;
  }
  int total = w();
  if (vertical_) {
    total = h();
  }
  std::vector<double> want(n, 0.0);
  double sum = 0;
  int known = 0;
  for (int i = 0; i < n; i++) {
    Fl_Dockable_Place *p = fl_dockable_first_panel(part(i))->place();
    want[i] = p->extent;
    if (level_ == 1) {
      want[i] = p->cross_extent;
    }
    if (want[i] > 0) {
      sum += want[i];
      known++;
    }
  }
  double mean = (double)total / n;
  if (known > 0) {
    mean = sum / known;
  }
  sum = 0;
  for (int i = 0; i < n; i++) {
    if (want[i] <= 0) {
      want[i] = mean;
    }
    sum += want[i];
  }
  int at = 0;
  for (int i = 0; i < n; i++) {
    int size = (int)(total * want[i] / sum + 0.5);
    if (i == n - 1) {
      size = total - at;
    }
    if (vertical_) {
      part(i)->resize(x(), y() + at, w(), size);
    } else {
      part(i)->resize(x() + at, y(), size, h());
    }
    at += size;
  }
  init_sizes();
  redraw();
}

// Only a panel dragged out of the stack comes here; the area takes the drops
int Fl_Dockable_Stack::handle(int event) {
  if (event == FL_UNDOCK) {
    bool processed = false;
    int r = Fl_Dockable_Group::handle_docking_target(processed, this, event, inside_cb, nullptr,
                                                     nullptr, payload_cb, undock_cb);
    if (processed) {
      return r;
    }
  }
  int r = Fl_Tile::handle(event);
  // The border between parts may have been dragged; the stack, not the area, had the mouse
  if (event == FL_RELEASE) {
    area_->borders_dragged();
  }
  return r;
}

bool Fl_Dockable_Stack::inside_cb(Fl_Widget *target) {
  return false;
}

void Fl_Dockable_Stack::payload_cb(Fl_Widget *target, Fl_Dockable_Group *dockable) {}

void Fl_Dockable_Stack::undock_cb(Fl_Widget *target, Fl_Dockable_Group *dockable) {
  Fl_Dockable_Stack *stack = (Fl_Dockable_Stack *)target;

  // FLTK leaves a Docking_Target_Box where the panel was; the stack closes up instead
  for (int i = stack->children() - 1; i >= 0; i--) {
    Fl_Widget *c = stack->child(i);
    if (!fl_dockable_is_part(c)) {
      stack->remove(c);
      Fl::delete_widget(c);
      break;
    }
  }
  // The layout is saved when the drag ends, with the panel's window where it was left
  stack->area_->unstacked(stack);
}
