//
// Title bar for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#include "Fl_Dockable_Bar.H"
#include <FL/Fl_Dockable_Panel.H>
#include <FL/Fl.H>
#include <FL/fl_draw.H>

Fl_Dockable_Bar::Fl_Dockable_Bar(Fl_Dockable_Panel *panel, int X, int Y, int W, int H)
    : Fl_Box(X, Y, W, H) {
  panel_ = panel;
  box(FL_FLAT_BOX);
  labelfont(FL_HELVETICA_BOLD);
  labelsize(13);
}

void Fl_Dockable_Bar::close_box(int &bx, int &by, int &bs) {
  bs = h() - 6;
  by = y() + 3;
  bx = x() + w() - 3 - bs;
}

// Left of the close button, or at the right end of a panel that cannot be closed
void Fl_Dockable_Bar::menu_box(int &bx, int &by, int &bs) {
  close_box(bx, by, bs);
  if (panel_->dock_flags() & FL_DOCKABLE_CLOSABLE) {
    bx -= 3 + bs;
  }
}

// The buttons are drawn flat on the bar, and the title short of them
void Fl_Dockable_Bar::draw() {
  draw_box();
  int bx, by, bs;
  if (panel_->dock_flags() & FL_DOCKABLE_CLOSABLE) {
    close_box(bx, by, bs);
    fl_draw_symbol("@-21+", bx + 2, by + 2, bs - 4, bs - 4, labelcolor());
  }
  menu_box(bx, by, bs);
  fl_draw_symbol("@-22>", bx + 2, by + 2, bs - 4, bs - 4, labelcolor());
  int right = bx;
  if (label()) {
    fl_font(labelfont(), labelsize());
    fl_color(labelcolor());
    fl_push_clip(x() + 6, y(), right - x() - 10, h());
    fl_draw(label(), x() + 6, y(), right - x() - 10, h(), FL_ALIGN_LEFT);
    fl_pop_clip();
  }
}

// A press on a button acts on it; anywhere else it starts a drag of the panel
int Fl_Dockable_Bar::handle(int event) {
  Fl_Dockable_Panel *d = panel_;
  if (event == FL_PUSH) {
    int bx, by, bs;
    int ex = Fl::event_x();
    // The menu button, or the right mouse button anywhere, opens the menu
    menu_box(bx, by, bs);
    if (Fl::event_button() == FL_RIGHT_MOUSE || (ex >= bx && ex < bx + bs)) {
      d->show_menu();
      return 1;
    }
    close_box(bx, by, bs);
    if ((d->dock_flags() & FL_DOCKABLE_CLOSABLE) && ex >= bx && ex < bx + bs) {
      d->user_close();
      return 1;
    }
  }
  if (event == FL_ENTER || event == FL_LEAVE) {
    return 1;
  }
  // A panel that cannot both move and float is not dragged
  int drag = FL_DOCKABLE_MOVABLE | FL_DOCKABLE_FLOATABLE;
  if (d->drag_widget() == this && (d->dock_flags() & drag) == drag) {
    int r = d->handle_drag_widget(event);
    if (event == FL_RELEASE) {
      d->released();
    }
    if (r) {
      return r;
    }
  }
  return Fl_Box::handle(event);
}
