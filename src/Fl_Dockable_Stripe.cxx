//
// Stripe for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#include "Fl_Dockable_Stripe.H"
#include <FL/Fl_Dockable_Panel.H>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Image.H>

#include <stdlib.h>

Fl_Dockable_Stripe::Fl_Dockable_Stripe(Fl_Dockable_Area *area, int side) : Fl_Widget(0, 0, 0, 0) {
  area_ = area;
  side_ = side;
  pressed_ = nullptr;
  press_x_ = 0;
  press_y_ = 0;
  dragging_ = false;
  box(FL_FLAT_BOX);
}

// A button along the stripe for each panel, as long as its title, across the whole stripe
void Fl_Dockable_Stripe::button_box(std::vector<Fl_Dockable_Panel *> &panels, int i, int &X,
                                    int &Y, int &W, int &H) {
  fl_font(FL_HELVETICA, 12);
  int at = 4;
  int len = 0;
  for (int k = 0; k <= i; k++) {
    len = (int)fl_width(panels[k]->title()) + 16;
    Fl_Image *icon = panels[k]->dock_icon();
    if (icon) {
      len += icon->w() + 4;
    }
    if (k < i) {
      at += len + 2;
    }
  }
  if (side_ == FL_DOCKABLE_LEFT || side_ == FL_DOCKABLE_RIGHT) {
    X = x() + 1;
    Y = y() + at;
    W = w() - 2;
    H = len;
  } else {
    X = x() + at;
    Y = y() + 1;
    W = len;
    H = h() - 2;
  }
}

Fl_Dockable_Panel *Fl_Dockable_Stripe::button_at(int X, int Y) {
  std::vector<Fl_Dockable_Panel *> panels = area_->stripe_panels(side_);
  for (size_t i = 0; i < panels.size(); i++) {
    int bx, by, bw, bh;
    button_box(panels, (int)i, bx, by, bw, bh);
    if (X >= bx && Y >= by && X < bx + bw && Y < by + bh) {
      return panels[i];
    }
  }
  return nullptr;
}

// Before the first button whose middle is past X, Y, between its neighbors' orders
double Fl_Dockable_Stripe::order_at(int X, int Y) {
  std::vector<Fl_Dockable_Panel *> panels = area_->stripe_panels(side_);
  bool vertical = (side_ == FL_DOCKABLE_LEFT || side_ == FL_DOCKABLE_RIGHT);
  size_t k = 0;
  while (k < panels.size()) {
    int bx, by, bw, bh;
    button_box(panels, (int)k, bx, by, bw, bh);
    if ((vertical && Y < by + bh / 2) || (!vertical && X < bx + bw / 2)) {
      break;
    }
    k++;
  }
  if (panels.empty()) {
    return 0;
  }
  if (k == 0) {
    return panels[0]->place()->stripe_order - 10;
  }
  if (k >= panels.size()) {
    return panels.back()->place()->stripe_order + 10;
  }
  return 0.5 * (panels[k - 1]->place()->stripe_order + panels[k]->place()->stripe_order);
}

void Fl_Dockable_Stripe::draw() {
  draw_box();
  std::vector<Fl_Dockable_Panel *> panels = area_->stripe_panels(side_);
  bool vertical = (side_ == FL_DOCKABLE_LEFT || side_ == FL_DOCKABLE_RIGHT);
  for (size_t i = 0; i < panels.size(); i++) {
    int bx, by, bw, bh;
    button_box(panels, (int)i, bx, by, bw, bh);
    Fl_Boxtype b = FL_UP_BOX;
    if (panels[i]->panel_shown()) {
      b = FL_DOWN_BOX;
    }
    draw_box(b, bx, by, bw, bh, FL_BACKGROUND_COLOR);
    fl_color(FL_FOREGROUND_COLOR);
    fl_font(FL_HELVETICA, 12);
    const char *text = panels[i]->title();
    // The icon first, then the title, read from the bottom up on a side stripe
    Fl_Image *icon = panels[i]->dock_icon();
    int skip = 0;
    if (icon) {
      skip = icon->w() + 4;
      if (vertical) {
        icon->draw(bx + (bw - icon->w()) / 2, by + bh - 6 - icon->h());
      } else {
        icon->draw(bx + 6, by + (bh - icon->h()) / 2);
      }
    }
    if (vertical) {
      fl_draw(90, text, bx + (bw + fl_height()) / 2 - fl_descent(), by + bh - 8 - skip);
    } else {
      fl_draw(text, bx + 8 + skip, by + (bh + fl_height()) / 2 - fl_descent());
    }
  }
}

// A button's menu: auto-hide or pin its panel, move the button to another stripe, or remove it
void Fl_Dockable_Stripe::show_menu(Fl_Dockable_Panel *d) {
  enum { BMENU_AUTOHIDE = 1, BMENU_LEFT, BMENU_RIGHT, BMENU_TOP, BMENU_BOTTOM, BMENU_REMOVE };
  Fl_Menu_Item items[] = {{"Auto-hide", 0, 0, (void *)BMENU_AUTOHIDE, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Move to Left Stripe", 0, 0, (void *)BMENU_LEFT, 0, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Move to Right Stripe", 0, 0, (void *)BMENU_RIGHT, 0, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Move to Top Stripe", 0, 0, (void *)BMENU_TOP, 0, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Move to Bottom Stripe", 0, 0, (void *)BMENU_BOTTOM, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Remove from Stripe", 0, 0, (void *)BMENU_REMOVE, 0, FL_NORMAL_LABEL, 0, 0, 0},
                          {0, 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 0, 0}};
  Fl_Dockable_Place *p = d->place();
  if (p->auto_hide) {
    items[0].label("Pin");
  }
  if (!area_->captive_layer()) {
    items[0].deactivate();
  }
  items[1 + side_].deactivate();
  const Fl_Menu_Item *m = items->popup(Fl::event_x(), Fl::event_y());
  if (!m) {
    return;
  }
  long choice = (long)m->user_data();
  if (choice == BMENU_AUTOHIDE) {
    if (p->auto_hide) {
      d->pin();
    } else {
      d->auto_hide();
    }
  } else if (choice >= BMENU_LEFT && choice <= BMENU_BOTTOM) {
    int side = (int)(choice - BMENU_LEFT);
    area_->move_button(d, side, area_->stripe_end(side));
  } else if (choice == BMENU_REMOVE) {
    d->show_button(false);
  }
}

// A click shows or hides the button's panel, through its open_request() and hide_request(); a drag
// moves the button
int Fl_Dockable_Stripe::handle(int event) {
  switch (event) {
    case FL_PUSH: {
      Fl_Dockable_Panel *d = button_at(Fl::event_x(), Fl::event_y());
      if (!d) {
        break;
      }
      if (Fl::event_button() == FL_RIGHT_MOUSE) {
        show_menu(d);
        return 1;
      }
      pressed_ = d;
      press_x_ = Fl::event_x_root();
      press_y_ = Fl::event_y_root();
      dragging_ = false;
      return 1;
    }
    case FL_DRAG: {
      if (!pressed_) {
        break;
      }
      // A panel that cannot move keeps its button where it is
      if (!dragging_ && (pressed_->dock_flags() & FL_DOCKABLE_MOVABLE) &&
          (abs(Fl::event_x_root() - press_x_) > 4 || abs(Fl::event_y_root() - press_y_) > 4)) {
        dragging_ = true;
        window()->cursor(FL_CURSOR_MOVE);
        area_->show_all_stripes(true);
      }
      if (dragging_) {
        area_->drag_button_over(pressed_, Fl::event_x_root(), Fl::event_y_root());
      }
      return 1;
    }
    case FL_RELEASE: {
      Fl_Dockable_Panel *d = pressed_;
      pressed_ = nullptr;
      if (!d) {
        break;
      }
      if (dragging_) {
        dragging_ = false;
        window()->cursor(FL_CURSOR_DEFAULT);
        int X, Y;
        int side = area_->stripe_at(Fl::event_x_root(), Fl::event_y_root(), X, Y);
        double order = 0;
        Fl_Dockable_Stripe *target = nullptr;
        if (side >= 0) {
          target = area_->stripe(side);
          order = target->order_at(X, Y);
        }
        area_->drag_button_over(nullptr, 0, 0);
        area_->show_all_stripes(false);
        if (target) {
          area_->move_button(d, side, order);
        } else {
          area_->drop_panel_at(d, Fl::event_x_root(), Fl::event_y_root());
        }
        return 1;
      }
      // With FL_DOCKABLE_STRIPE_FOCUS_FIRST, a click on a panel shown without the keyboard focus gives
      // it the focus rather than hiding it
      bool focused = Fl::focus() && d->contains(Fl::focus());
      if (d->panel_shown() && (area_->stripe_options() & FL_DOCKABLE_STRIPE_FOCUS_FIRST) && !focused) {
        d->take_focus();
        return 1;
      }
      area_->request(d, !d->panel_shown());
      return 1;
    }
    default:
      break;
  }
  return Fl_Widget::handle(event);
}
