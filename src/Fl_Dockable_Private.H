//
// Private helpers for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#ifndef Fl_Dockable_Private_H
#define Fl_Dockable_Private_H

#include <FL/Fl_Dockable_Area.H>
#include <FL/Fl_Dockable_Panel.H>
#include "Fl_Dockable_Tabs.H"
#include "Fl_Dockable_Stack.H"

#include <vector>

// What a panel dragged over a docked one would do: join it as a tab, stack before or after it along
// the place, stack before or after it across the place, or, over a stripe, auto-hide there
enum {
  FL_DOCKABLE_HINT_TAB = 0,
  FL_DOCKABLE_HINT_BEFORE,
  FL_DOCKABLE_HINT_AFTER,
  FL_DOCKABLE_HINT_ACROSS_BEFORE,
  FL_DOCKABLE_HINT_ACROSS_AFTER,
  FL_DOCKABLE_HINT_STRIPE
};

// Columns at the left and right, rows at the top and bottom
static inline bool fl_dockable_column_side(int side) {
  return side == FL_DOCKABLE_LEFT || side == FL_DOCKABLE_RIGHT;
}

// A part of a stack: a panel, tabs, or a stack across it
static inline bool fl_dockable_is_part(Fl_Widget *c) {
  return dynamic_cast<Fl_Dockable_Panel *>(c) || dynamic_cast<Fl_Dockable_Tabs *>(c) ||
         dynamic_cast<Fl_Dockable_Stack *>(c);
}

// Every panel docked in a place, in its tabs or stacked
static inline void fl_dockable_panels_in(Fl_Widget *c, std::vector<Fl_Dockable_Panel *> &out) {
  Fl_Dockable_Panel *d = dynamic_cast<Fl_Dockable_Panel *>(c);
  if (d) {
    out.push_back(d);
    return;
  }
  if (dynamic_cast<Fl_Dockable_Tabs *>(c) || dynamic_cast<Fl_Dockable_Stack *>(c)) {
    Fl_Group *g = c->as_group();
    for (int i = 0; i < g->children(); i++) {
      fl_dockable_panels_in(g->child(i), out);
    }
  }
}

// The panel docked in a place along a side, or the first of its tabs or stack
static inline Fl_Dockable_Panel *fl_dockable_first_panel(Fl_Widget *c) {
  std::vector<Fl_Dockable_Panel *> panels;
  fl_dockable_panels_in(c, panels);
  if (panels.empty()) {
    return 0;
  }
  return panels[0];
}

// The place of a panel docked along a side; the panels in one place share its side, order and size
static inline Fl_Dockable_Place *fl_dockable_place_of(Fl_Widget *c) {
  return fl_dockable_first_panel(c)->place();
}

// The width, or height, a panel wants along a side: its place's, or what it was built at
static inline int fl_dockable_want(Fl_Dockable_Panel *d, int side) {
  Fl_Dockable_Place *p = d->place();
  if (fl_dockable_column_side(side)) {
    if (p->width > 0) {
      return p->width;
    }
    return d->built_w();
  }
  if (p->height > 0) {
    return p->height;
  }
  return d->built_h();
}

// Moves every child of one group to another, with its resizable child
static inline void fl_dockable_move_children(Fl_Group *from, Fl_Group *to) {
  Fl_Widget *resize_widget = from->resizable();
  while (from->children() > 0) {
    to->add(from->child(0));
  }
  if (resize_widget == from || !resize_widget) {
    to->resizable(to);
  } else {
    to->resizable(resize_widget);
  }
  from->resizable(from);
  to->init_sizes();
}

#endif // !Fl_Dockable_Private_H
