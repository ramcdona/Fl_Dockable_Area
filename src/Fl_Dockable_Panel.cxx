//
// Dockable panel for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#include <FL/Fl_Dockable_Panel.H>
#include "Fl_Dockable_Bar.H"
#include "Fl_Dockable_Tabs.H"
#include "Fl_Dockable_Stack.H"
#include "Fl_Dockable_Private.H"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <stdio.h>

/**
  Creates a panel of \p area whose body is \p W by \p H, below a title bar
  showing \p title, in \p place, or in a place of its own named by the title
  if that is 0.  The panel is hidden until show_panel() is called, and is
  not added to the current group.  Add widgets to it between begin_body() and
  end_body(), or with adopt().
*/
Fl_Dockable_Panel::Fl_Dockable_Panel(Fl_Dockable_Area *area, int W, int H, const char *title,
                                     Fl_Dockable_Place *place)
    : Fl_Dockable_Group(0, 0, W, H + bar_height()) {
  // A panel goes where its place says, not in the group current as it is made, which is current
  // again after it
  end();
  Fl_Group *cur = Fl_Group::current();
  if (parent()) {
    parent()->remove(this);
  }

  area_ = area;
  own_place_ = false;
  flags_ = FL_DOCKABLE_DEFAULT_FLAGS;
  shortcut_ = 0;
  icon_ = 0;
  built_w_ = w();
  built_h_ = h();
  bar_shown_ = true;
  settled_ = false;
  saved_current_ = 0;

  float_w_ = 0;
  float_h_ = 0;
  slide_x_ = 0;
  slide_y_ = 0;
  lent_x_ = 0;
  lent_y_ = 0;
  lent_w_ = 0;
  lent_h_ = 0;

  title_ = "";
  if (title) {
    title_ = title;
  }
  // A place of its own, named by the title, or the title and a number if that is taken
  if (!place) {
    std::string n = title_;
    for (int k = 2; area->find_place(n.c_str()); k++) {
      char buf[32];
      snprintf(buf, sizeof(buf), " %d", k);
      n = title_ + buf;
    }
    place = area->add_place(n.c_str());
  }
  place_ = place;

  // The body's widgets are placed relative to its corner at 0, 0, as in a window; it moves below the
  // title bar when the panel is first shown
  Fl_Group::current(this);
  body_ = new Fl_Group(0, 0, W, H);
  body_->end();
  body_->box(FL_NO_BOX);
  bar_ = new Fl_Dockable_Bar(this, 0, 0, W, bar_height());
  Fl_Group::current(cur);
  resizable(body_);

  // Its label names its tab when it is docked as one; inside the group, the body's widgets cover it
  align(FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
  copy_label(title_.c_str());
  label_for_states(title_.c_str(), title_.c_str(), title_.c_str());
  color_for_states(FL_DARK3, FL_DARK3, FL_SELECTION_COLOR);
  drag_widget(bar_);
  // Released over the area's captive layer, it floats captive there
  captive(area->captive_layer());
  area->register_panel(this);
}

/** Takes the panel out of its area and its window. */
Fl_Dockable_Panel::~Fl_Dockable_Panel() {
  if (area_) {
    area_->forget_panel(this);
  }
  if (own_place_) {
    delete place_;
  }
  Fl_Window *win = window();
  if (parent() && win && parent() == win && (!area_ || win != area_->window())) {
    win->remove(this);
    win->hide();
    Fl::delete_widget(win);
  } else if (parent()) {
    parent()->remove(this);
  }
}

/** Makes the body the current group, so the widgets created next go into it. */
void Fl_Dockable_Panel::begin_body() {
  saved_current_ = Fl_Group::current();
  body_->begin();
}

/** Makes the group current before begin_body() current again. */
void Fl_Dockable_Panel::end_body() {
  Fl_Group::current(saved_current_);
  saved_current_ = 0;
}

/**
  Moves the widgets of \p from, such as a window the application built, into
  the body, which takes its size; \p from keeps none.  Call it before the
  panel is first shown.
*/
void Fl_Dockable_Panel::adopt(Fl_Group *from) {
  int bh = bar_height();
  Fl_Widget::resize(x(), y(), from->w(), from->h() + bh);
  body_->Fl_Widget::resize(0, 0, from->w(), from->h());
  bar_->Fl_Widget::resize(0, 0, from->w(), bh);
  fl_dockable_move_children(from, body_);
  built_w_ = w();
  built_h_ = h();
}

// The body moves below the title bar, with its widgets, the first time the panel is placed
void Fl_Dockable_Panel::settle() {
  if (settled_) {
    return;
  }
  settled_ = true;
  init_sizes();
  hide_subwindows();
  resize(x(), y(), w(), h());
  show_subwindows();
}

/** Sets the key that shows and hides the panel, such as FL_ALT + '1', from any window. */
void Fl_Dockable_Panel::dock_shortcut(int s) {
  shortcut_ = s;
  if (area_) {
    area_->update_panel_menu();
  }
}

/** Sets the panel's place; shown, it moves there when next shown. */
void Fl_Dockable_Panel::place(Fl_Dockable_Place *p) {
  if (!p || p == place_ || !area_) {
    return;
  }
  bool shown = panel_shown();
  if (shown) {
    hide_panel();
  }
  place_ = p;
  if (shown) {
    show_panel();
  }
}

/** The name the panel's sizes are saved under: the name set, or else the title. */
const char *Fl_Dockable_Panel::name() const {
  if (!name_.empty()) {
    return name_.c_str();
  }
  return title_.c_str();
}

// Back to the docked state, out of a window: FLTK has no call for this but setting the drag widget
void Fl_Dockable_Panel::rearm() {
  Fl_Widget *drag = drag_widget();
  drag_widget(0);
  drag_widget(drag);
}

// Every visible subwindow under a group
void Fl_Dockable_Panel::find_subwindows(Fl_Group *g, std::vector<Fl_Window *> &wins) {
  for (int i = 0; i < g->children(); i++) {
    Fl_Widget *c = g->child(i);
    if (c->as_window()) {
      if (c->visible()) {
        wins.push_back(c->as_window());
      }
    } else if (c->as_group()) {
      find_subwindows(c->as_group(), wins);
    }
  }
}

// Hides the subwindows in the group, before it moves to another window
void Fl_Dockable_Panel::hide_subwindows() {
  if (!subwindows_.empty()) {
    return;
  }
  find_subwindows(this, subwindows_);
  for (size_t i = 0; i < subwindows_.size(); i++) {
    subwindows_[i]->hide();
  }
}

// Shows them again in the window the group is in now, and any that window has not shown yet, as
// when the group is first put in it
void Fl_Dockable_Panel::show_subwindows() {
  Fl_Window *win = window();
  bool shown = (win && win->shown() && parent());
  for (size_t i = 0; i < subwindows_.size(); i++) {
    if (shown) {
      subwindows_[i]->show();
    } else {
      subwindows_[i]->set_visible();
    }
  }
  subwindows_.clear();

  if (shown) {
    std::vector<Fl_Window *> subs;
    find_subwindows(this, subs);
    for (size_t i = 0; i < subs.size(); i++) {
      if (!subs[i]->shown() && subs[i]->visible_r()) {
        subs[i]->show();
      }
    }
  }
}

/**
  Draws the panel with a dark frame over the margin of its widgets, which sets
  it apart from the panels beside it.
*/
void Fl_Dockable_Panel::draw() {
  Fl_Dockable_Group::draw();
  fl_color(FL_DARK3);
  fl_rect(x(), y(), w(), h());
}

/**
  Resizes the panel: the title bar takes the top while it is shown, and the
  body the rest.  The application hears of the body's new size through
  body_resized().
*/
void Fl_Dockable_Panel::resize(int X, int Y, int W, int H) {
  Fl_Widget::resize(X, Y, W, H);
  int bh = 0;
  if (bar_shown_) {
    bh = Fl_Dockable_Panel::bar_height();
  }
  bar_->resize(X, Y, W, Fl_Dockable_Panel::bar_height());
  body_->resize(X, Y + bh, W, H - bh);
  if (settled_) {
    body_resized();
  }
}

/**
  Sets the title its title bar, tab, stripe button, window and the panel menu
  show.  The panel keeps a copy.
*/
void Fl_Dockable_Panel::title(const char *title) {
  title_ = "";
  if (title) {
    title_ = title;
  }
  copy_label(title_.c_str());
  if (parent()) {
    parent()->redraw();
  }
  bar_->redraw();
  if (drag_widget()) {
    label_for_states(title_.c_str(), title_.c_str(), title_.c_str());
  }
  if (area_) {
    area_->update_panel_menu();
  }
  if (window() && !window()->parent() && (!area_ || window() != area_->window())) {
    window()->copy_label(title_.c_str());
  }
}

/**
  Keeps the panel's place up to date as FLTK undocks and docks it: undocked,
  its place floats, and its window is titled and closes the panel as its close
  button does; docked, it does not auto-hide.
*/
int Fl_Dockable_Panel::handle(int event) {
  if (!area_) {
    return Fl_Dockable_Group::handle(event);
  }
  if (event == FL_UNDOCK) {
    place_->docked = false;
    in_window();
    built_size();
  } else if (event == FL_DOCK_RELEASE) {
    // Docked, it stops auto-hiding; its button stays.  Dropped on a stripe, it has only its
    // button there.
    if (docked()) {
      place_->docked = true;
      if (place_->auto_hide) {
        place_->auto_hide = false;
        area_->slid_away(this);
      }
    }
  }
  return Fl_Dockable_Group::handle(event);
}

// The window FLTK made for it closes the panel as its close button does, and is titled as it is
void Fl_Dockable_Panel::in_window() {
  if (!area_) {
    return;
  }
  Fl_Window *win = window();
  if (win && win != area_->window()) {
    win->callback(window_cb, this);
    win->copy_label(title_.c_str());
    // Above the area's window, as a tool window is.  FLTK shows the window it makes for a
    // group dragged out before the application sees it; macOS sets the level when the window is
    // next activated.
    if (!win->parent()) {
      win->set_non_modal();
    }
  }
}

// A panel floated out of the area, the height of the area's window, takes the size it was built at,
// within the work area of its screen
void Fl_Dockable_Panel::built_size() {
  Fl_Window *win = window();
  if (!win || parent() != win) {
    return;
  }
  int mx = x();
  int my = y();
  int sx, sy, sw, sh;
  Fl::screen_work_area(sx, sy, sw, sh, win->screen_num());
  // Floating captive, it stays within the group it floats in
  if (win->parent()) {
    sh = win->parent()->h();
  }
  int W = built_w_ + 2 * mx;
  int H = built_h_ + 2 * my;
  // One that floated before takes the size it last floated at instead
  if (float_w_ > 0 && float_h_ > 0) {
    W = float_w_;
    H = float_h_;
  }
  if (H > sh) {
    H = sh;
  }
  win->size(W, H);
}

void Fl_Dockable_Panel::window_cb(Fl_Widget *w, void *data) {
  Fl_Dockable_Panel *d = (Fl_Dockable_Panel *)data;
  if (d->dock_flags() & FL_DOCKABLE_CLOSABLE) {
    d->user_close();
  }
}

// The user closed the panel: the application closes it, and is told
void Fl_Dockable_Panel::user_close() {
  Fl_Dockable_Area *area = area_;
  Fl_Widget_Tracker alive(this);
  close_request();
  if (area) {
    Fl_Dockable_Panel *d = this;
    if (alive.deleted()) {
      d = 0;
    }
    area->notify(d, FL_REASON_CLOSED);
  }
}

/**
  Lends the body's widgets to \p to, such as the window the application built
  them in, placed as they were built, so the application can draw them there
  alone, as for a screen dump; take_back_body() takes them back.
*/
void Fl_Dockable_Panel::lend_body(Fl_Group *win) {
  lent_x_ = body_->x();
  lent_y_ = body_->y();
  lent_w_ = body_->w();
  lent_h_ = body_->h();
  hide_subwindows();

  // The window's widgets were placed relative to its corner, as they are again with the group at 0,
  // 0
  body_->resize(0, 0, win->w(), win->h());
  fl_dockable_move_children(body_, win);

  for (size_t i = 0; i < subwindows_.size(); i++) {
    subwindows_[i]->set_visible();
  }
}

/** Takes the widgets lent to \p win back, where they were. */
void Fl_Dockable_Panel::take_back_body(Fl_Group *win) {
  for (size_t i = 0; i < subwindows_.size(); i++) {
    subwindows_[i]->hide();
  }
  win->hide();

  // Back in the group at the window's corner, where they were placed, then where the group was
  body_->Fl_Widget::resize(0, 0, win->w(), win->h());
  fl_dockable_move_children(win, body_);
  body_->resize(lent_x_, lent_y_, lent_w_, lent_h_);
  show_subwindows();
  redraw();
}

// The bar is hidden in tabs, whose tab is its title, and the panel's widgets take its room
void Fl_Dockable_Panel::show_bar(bool on) {
  if (on == bar_shown_) {
    return;
  }
  bar_shown_ = on;
  if (on) {
    bar_->show();
  } else {
    bar_->hide();
  }
  resize(x(), y(), w(), h());
  redraw();
}

/** Returns true if the panel is docked in its area, alone, as a tab or in a stack. */
int Fl_Dockable_Panel::docked() const {
  for (Fl_Widget *p = parent(); p; p = p->parent()) {
    if (p == area_) {
      return true;
    }
    if (!dynamic_cast<Fl_Dockable_Tabs *>(p) && !dynamic_cast<Fl_Dockable_Stack *>(p)) {
      return false;
    }
  }
  return false;
}

// After a drag, where a floating window was left is kept
void Fl_Dockable_Panel::released() {
  if (!area_) {
    return;
  }
  // Dragged away from where it slid out, it stops auto-hiding, and floats there
  if (place_->auto_hide && parent() && !docked()) {
    if (window()->x() == slide_x_ && window()->y() == slide_y_) {
      return;
    }
    place_->auto_hide = false;
    place_->docked = false;
    area_->slid_away(this);
  }
  if (parent() && !docked()) {
    keep_float_place();
    area_->notify(this, FL_REASON_DRAGGED);
  }
}

void Fl_Dockable_Panel::keep_float_place() {
  Fl_Window *win = window();
  if (win && parent() == win && win->shown()) {
    place_->float_set = true;
    place_->float_captive = (win->parent() != nullptr);
    place_->float_x = win->x();
    place_->float_y = win->y();
    // Its size too
    float_w_ = win->w();
    float_h_ = win->h();
  }
}

// The item of a menu whose user data is id
static Fl_Menu_Item *menu_item_of(Fl_Menu_Item *items, long id) {
  for (int i = 0; items[i].label(); i++) {
    if ((long)items[i].user_data() == id) {
      return &items[i];
    }
  }
  return nullptr;
}

// The menu of the panel's title: dock it along a border, float it in the area's window or in a
// window of its own, or close it
void Fl_Dockable_Panel::show_menu() {
  if (!area_) {
    return;
  }
  enum {
    MENU_LEFT = 1,
    MENU_RIGHT,
    MENU_TOP,
    MENU_BOTTOM,
    MENU_INSIDE,
    MENU_WINDOW,
    MENU_AUTOHIDE,
    MENU_BUTTON,
    MENU_MAXIMIZE,
    MENU_CLOSE
  };
  bool in_area = docked();
  bool floating = (parent() && !in_area);
  bool captive = (floating && window()->parent());
  int side = -1;
  if (in_area) {
    side = place_->side;
  }

  Fl_Menu_Item items[] = {{"Dock Left", 0, 0, (void *)MENU_LEFT, 0, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Dock Right", 0, 0, (void *)MENU_RIGHT, 0, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Dock Top", 0, 0, (void *)MENU_TOP, 0, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Dock Bottom", 0, 0, (void *)MENU_BOTTOM, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Float Inside", 0, 0, (void *)MENU_INSIDE, 0, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Float in Own Window", 0, 0, (void *)MENU_WINDOW, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Auto-hide on Stripe", 0, 0, (void *)MENU_AUTOHIDE, 0, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Show Button on Stripe", 0, 0, (void *)MENU_BUTTON, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Maximize", 0, 0, (void *)MENU_MAXIMIZE, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 0, 0},
                          {"Close", 0, 0, (void *)MENU_CLOSE, 0, FL_NORMAL_LABEL, 0, 0, 0},
                          {0, 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 0, 0}};
  bool striped = place_->auto_hide;
  if (striped) {
    menu_item_of(items, MENU_AUTOHIDE)->label("Pin");
  }
  if (!area_->captive_layer()) {
    menu_item_of(items, MENU_AUTOHIDE)->deactivate();
  }
  bool button = (place_->stripe >= 0);
  if (button) {
    menu_item_of(items, MENU_BUTTON)->label("Remove Button from Stripe");
  }
  bool maximized = in_area && area_->maximized(this);
  if (maximized) {
    menu_item_of(items, MENU_MAXIMIZE)->label("Restore Size");
  }
  if (!in_area) {
    menu_item_of(items, MENU_MAXIMIZE)->deactivate();
  }
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    if (s == side || !area_->fits(this, s)) {
      menu_item_of(items, MENU_LEFT + s)->deactivate();
    }
  }
  if (captive || !area_->captive_layer()) {
    menu_item_of(items, MENU_INSIDE)->deactivate();
  }
  if (floating && !captive) {
    menu_item_of(items, MENU_WINDOW)->deactivate();
  }
  // What the panel's flags do not allow
  if (!(flags_ & FL_DOCKABLE_MOVABLE)) {
    for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
      menu_item_of(items, MENU_LEFT + s)->deactivate();
    }
  }
  if (!(flags_ & FL_DOCKABLE_FLOATABLE)) {
    menu_item_of(items, MENU_INSIDE)->deactivate();
    menu_item_of(items, MENU_WINDOW)->deactivate();
  }
  if (!(flags_ & FL_DOCKABLE_CLOSABLE)) {
    menu_item_of(items, MENU_CLOSE)->deactivate();
  }

  const Fl_Menu_Item *m = items->popup(Fl::event_x(), Fl::event_y());
  if (!m) {
    return;
  }
  long choice = (long)m->user_data();
  if (choice >= MENU_LEFT && choice <= MENU_BOTTOM) {
    dock_on((int)(choice - MENU_LEFT));
  } else if (choice == MENU_INSIDE || choice == MENU_WINDOW) {
    float_as(choice == MENU_INSIDE);
  } else if (choice == MENU_AUTOHIDE) {
    if (striped) {
      pin();
    } else {
      auto_hide();
    }
  } else if (choice == MENU_BUTTON) {
    show_button(!button);
  } else if (choice == MENU_MAXIMIZE) {
    if (maximized) {
      area_->maximize(nullptr);
    } else {
      area_->maximize(this);
    }
  } else if (choice == MENU_CLOSE) {
    user_close();
  }
}

// Slides out over the center in a captive window: one made for it, or the one it slid out in before
void Fl_Dockable_Panel::slide_out(int X, int Y, int W, int H) {
  if (!area_) {
    return;
  }
  Fl_Group *layer = area_->captive_layer();
  if (docked()) {
    hide_subwindows();
    area_->hide_column(this);
  }
  Fl_Window *win = window();
  if (parent() && win && !win->parent()) {
    // A window of its own goes; it slides out in the area's window
    hide_subwindows();
    win->hide();
    win->remove(this);
    Fl::delete_widget(win);
    rearm();
  }
  if (!parent()) {
    hide_subwindows();
    make_draggable(layer);
    in_window();
  }
  win = window();
  win->resize(X, Y, W, H);
  slide_x_ = X;
  slide_y_ = Y;
  win->show();
  show_subwindows();
}

/**
  Gives the panel a button at the end of the stripe of the side it is docked
  on, or the right, if \p on is true; or takes its button away, pinning it
  if it was auto-hidden.
*/
void Fl_Dockable_Panel::show_button(int on) {
  if (!area_) {
    return;
  }
  if (!on) {
    if (place_->auto_hide) {
      pin();
    }
    place_->stripe = -1;
    area_->relayout();
    area_->notify(this, FL_REASON_DRAGGED);
    return;
  }
  if (place_->stripe >= 0) {
    return;
  }
  int side = FL_DOCKABLE_RIGHT;
  if (docked()) {
    side = place_->side;
  }
  place_->stripe_order = area_->stripe_end(side);
  place_->stripe = side;
  area_->relayout();
  area_->notify(this, FL_REASON_DRAGGED);
}

// Takes it out of a hidden window it floated in, so it is next shown in a window made where its
// place says
void Fl_Dockable_Panel::unfloat() {
  if (!area_) {
    return;
  }
  Fl_Window *win = window();
  if (!parent() || docked() || !win) {
    return;
  }
  hide_subwindows();
  win->hide();
  win->remove(this);
  Fl::delete_widget(win);
  rearm();
}

/**
  Auto-hides the panel: it hides to its button, which it is given on the
  stripe of the side it is docked on, or the right, if it has none, and slides
  out from the button over the center when the button is pressed.
*/
void Fl_Dockable_Panel::auto_hide() {
  if (!area_) {
    return;
  }
  if (place_->stripe < 0) {
    int side = FL_DOCKABLE_RIGHT;
    if (docked()) {
      side = place_->side;
    }
    place_->stripe_order = area_->stripe_end(side);
    place_->stripe = side;
  }
  hide_panel();
  place_->auto_hide = true;
  area_->relayout();
  area_->notify(this, FL_REASON_DRAGGED);
}

/**
  Pins an auto-hidden panel: it shows where it was docked or floating, and
  keeps its button.
*/
void Fl_Dockable_Panel::pin() {
  if (!area_) {
    return;
  }
  bool shown = panel_shown();
  if (shown) {
    hide_panel();
  }
  place_->auto_hide = false;
  area_->relayout();
  if (shown) {
    show_panel();
  }
  area_->notify(this, FL_REASON_DRAGGED);
}

// Docks it along a border, next to the center
// Docked or floated from its menu, it stops auto-hiding; slid out, it stays where it slid out
// until it moves
void Fl_Dockable_Panel::stop_auto_hide() {
  if (!area_) {
    return;
  }
  if (place_->auto_hide) {
    place_->auto_hide = false;
    area_->slid_away(this);
  }
}

/**
  Docks the panel along \p side, an Fl_Dockable_Side, in a place of its own
  next to the center.
*/
void Fl_Dockable_Panel::dock_on(int side) {
  if (!area_) {
    return;
  }
  stop_auto_hide();
  place_->side = side;
  place_->order = area_->free_order(this, side, area_->order_next_to_view(side, this));
  if (docked()) {
    hide_subwindows();
    area_->hide_column(this);
  }
  dock_panel();
}

/**
  Floats the panel where it is: captive, in a window inside the area's window
  over its captive layer, if \p captive is true and the area has one, or else
  in a window of its own.  A floating panel moves between the two.
*/
void Fl_Dockable_Panel::float_as(int captive) {
  if (!area_) {
    return;
  }
  stop_auto_hide();
  Fl_Group *layer = area_->captive_layer();
  if (!layer) {
    captive = false;
  }
  Fl_Window *win = window();
  Fl_Window *main = area_->window();
  int X = 0;
  int Y = 0;
  if (docked()) {
    X = win->x_root() + x();
    Y = win->y_root() + y();
  } else if (parent()) {
    if ((win->parent() != nullptr) == captive) {
      return;
    }
    X = win->x_root();
    Y = win->y_root();
  }

  if (parent() && !docked()) {
    // The window it floats in moves into the layer, or out of it, as FLTK moves it when it is
    // released
    hide_subwindows();
    win->hide();
    if (win->parent()) {
      win->parent()->remove(win);
    }
    if (captive) {
      win->clear_modal_states();
      layer->add(win);
      win->position(X - main->x_root(), Y - main->y_root());
      layer->resize(layer->x(), layer->y(), layer->w(), layer->h());
    } else {
      win->set_non_modal();
      win->position(X, Y);
    }
    win->show();
    show_subwindows();
    keep_float_place();
    area_->notify(this, FL_REASON_DRAGGED);
    return;
  }

  if (captive) {
    X -= main->x_root();
    Y -= main->y_root();
  }
  float_panel(X, Y, captive);
}

/**
  Shows the panel where its place says: docked along its border, or floating
  if it does not fit or its place floats, or sliding out from its stripe
  button if it auto-hides.  A panel sharing its place with one shown takes its
  place, and a tab is brought to the front.
*/
void Fl_Dockable_Panel::show_panel() {
  if (!area_) {
    return;
  }
  show_in_place();
  area_->update_panel_menu();
}

void Fl_Dockable_Panel::show_in_place() {
  settle();
  place_->last = this;
  area_->redraw_stripes();
  if (place_->auto_hide && place_->stripe >= 0 && area_->captive_layer()) {
    area_->slide_out(this);
    return;
  }
  bool floating = (parent() && !docked());
  if (place_->docked && area_->fits(this, place_->side)) {
    if (floating) {
      dock_panel();
    } else {
      area_->show_column(this);
    }
    // A tab is brought to the front
    Fl_Dockable_Tabs *tabs = dynamic_cast<Fl_Dockable_Tabs *>(parent());
    if (tabs) {
      tabs->value(this);
    }
    return;
  }

  // Floating, or there is no room for its column this time
  if (floating) {
    keep_inside();
    window()->show();
    return;
  }
  int X, Y;
  bool captive = (area_->captive_layer() != nullptr);
  if (place_->float_set) {
    X = place_->float_x;
    Y = place_->float_y;
    captive = captive && place_->float_captive;
  } else {
    // Panels floated for want of room step down and right from the center's corner, so none hides
    // another
    int &cascade = area_->cascade_;
    X = area_->x() + 40 + 30 * cascade;
    Y = area_->y() + 40 + 30 * cascade;
    if (!captive) {
      X += area_->window()->x();
      Y += area_->window()->y();
    }
    cascade = (cascade + 1) % 10;
  }
  bool docked = place_->docked;
  float_panel(X, Y, captive);
  // A panel floated for want of room still docks when there is room
  place_->docked = docked;
}

/** Hides the panel, docked or floating; its place keeps where it was. */
void Fl_Dockable_Panel::hide_panel() {
  if (!area_) {
    return;
  }
  hide_from_place();
  area_->update_panel_menu();
}

void Fl_Dockable_Panel::hide_from_place() {
  area_->redraw_stripes();
  if (place_->auto_hide && parent() && !docked()) {
    window()->hide();
    area_->slid_away(this);
    return;
  }
  if (docked()) {
    hide_subwindows();
    area_->hide_column(this);
  } else if (parent()) {
    keep_float_place();
    window()->hide();
  }
}

/** Returns true if the panel is shown: docked, floating or slid out, in a window shown. */
int Fl_Dockable_Panel::panel_shown() const {
  if (!parent()) {
    return false;
  }
  Fl_Window *win = window();
  return win && win->shown();
}

// A floating window kept, size and all, inside the group it floats captive in, or else the work
// area of the screen it is on, as an area's window made smaller or a screen taken away would
// leave it out of reach
void Fl_Dockable_Panel::keep_inside() {
  Fl_Window *win = window();
  if (!win || parent() != win) {
    return;
  }
  int X, Y, W, H;
  if (win->parent()) {
    Fl_Group *g = win->parent();
    X = g->x();
    Y = g->y();
    W = g->w();
    H = g->h();
  } else {
    int n = Fl::screen_num(win->x() + win->w() / 2, win->y() + win->h() / 2);
    Fl::screen_work_area(X, Y, W, H, n);
  }
  int ww = std::min(win->w(), W);
  int wh = std::min(win->h(), H);
  int wx = std::max(X, std::min(win->x(), X + W - ww));
  int wy = std::max(Y, std::min(win->y(), Y + H - wh));
  if (wx != win->x() || wy != win->y() || ww != win->w() || wh != win->h()) {
    // Subwindows of a window not shown are hidden while it is resized
    bool wrap = (!win->shown() && subwindows_.empty());
    if (wrap) {
      hide_subwindows();
    }
    win->resize(wx, wy, ww, wh);
    if (wrap) {
      show_subwindows();
    }
  }
}

/**
  Floats the panel in a window at \p X, \p Y: captive, in a window inside
  the area's window over its captive layer, at X, Y in that window's
  coordinates, if \p captive is true, or else in a window of its own at X, Y
  on the screen.  A panel already floating is only shown.
*/
void Fl_Dockable_Panel::float_panel(int X, int Y, int captive) {
  if (!area_) {
    return;
  }
  settle();
  if (docked()) {
    hide_subwindows();
    area_->hide_column(this);
  }
  if (!parent()) {
    // make_draggable() puts it in a window FLTK makes, as dragging it out of the area does
    hide_subwindows();
    // A subwindow of the captive group, or a top-level window
    Fl_Group *in = nullptr;
    if (captive) {
      in = area_->captive_layer();
    }
    Fl_Window *win = make_draggable(in);
    in_window();
    built_size();
    win->position(X, Y);
  }
  keep_inside();
  window()->show();
  show_subwindows();
  place_->docked = false;
  keep_float_place();
  area_->notify(this, FL_REASON_DRAGGED);
}

/**
  Docks a floating panel back in its docked place, and closes the window it
  floated in.  A hidden panel is docked and shown.
*/
void Fl_Dockable_Panel::dock_panel() {
  if (!area_) {
    return;
  }
  Fl_Window *win = window();
  if (parent() && !docked() && win) {
    keep_float_place();
    hide_subwindows();
    win->hide();
    win->remove(this);
    Fl::delete_widget(win);
    // Back to the docked state: FLTK has no call for this but setting the drag widget
    rearm();
  }
  place_->docked = true;
  area_->show_column(this);
  area_->notify(this, FL_REASON_DRAGGED);
}
