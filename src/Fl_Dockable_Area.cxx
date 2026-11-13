//
// Docking area for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#include <FL/Fl_Dockable_Area.H>
#include <FL/Fl_Dockable_Panel.H>
#include "Fl_Dockable_Tabs.H"
#include "Fl_Dockable_Stack.H"
#include "Fl_Dockable_Stripe.H"
#include "Fl_Dockable_Private.H"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <string.h>
#include <math.h>

Fl_Dockable_Place::Fl_Dockable_Place() {
  side = FL_DOCKABLE_RIGHT;
  order = 0;
  width = 0;
  height = 0;
  sub = 0;
  extent = 0;
  cross = 0;
  cross_extent = 0;
  docked = true;
  stripe = -1;
  stripe_order = 0;
  auto_hide = false;
  shown = false;
  last = 0;
  float_set = false;
  float_captive = true;
  float_x = 0;
  float_y = 0;
}

// Thicknesses along two opposite sides scaled down to fit in room, if they do not
static bool fit_thickness(std::vector<int> th[], int t[], int a, int b, int room) {
  int total = t[a] + t[b];
  if (total <= room || total <= 0) {
    return false;
  }
  if (room < 0) {
    room = 0;
  }
  int sides[2] = {a, b};
  for (int k = 0; k < 2; k++) {
    int s = sides[k];
    t[s] = 0;
    for (size_t i = 0; i < th[s].size(); i++) {
      th[s][i] = (int)((double)th[s][i] * room / total);
      t[s] += th[s][i];
    }
  }
  return true;
}

// Every area, which the dispatch tells of clicks
static std::vector<Fl_Dockable_Area *> s_areas;
static Fl_Event_Dispatch s_prev_dispatch = 0;

/**
  Returns the version of the library linked, FL_DOCKABLE_AREA_API_VERSION as
  it was built.  An application compares it with FL_DOCKABLE_AREA_API_VERSION
  to find headers and a library that differ.
*/
int Fl_Dockable_Area::api_version() {
  return FL_DOCKABLE_AREA_API_VERSION;
}

/**
  The event dispatch the areas put in front of the one set before the first
  was made: a click anywhere but on a panel slid out, or its button, slides it
  away.  An application that sets its own Fl::event_dispatch() after making an
  area must call this.
*/
int Fl_Dockable_Area::dispatch(int event, Fl_Window *w) {
  if (event == FL_PUSH) {
    for (size_t i = 0; i < s_areas.size(); i++) {
      s_areas[i]->pushed_at(w);
    }
  }
  // Escape slides a panel slid out away, before any widget takes the key
  if (event == FL_KEYBOARD && Fl::event_key() == FL_Escape && !Fl::grab() && !Fl::modal()) {
    for (size_t i = 0; i < s_areas.size(); i++) {
      if (s_areas[i]->slid_) {
        s_areas[i]->key_pressed();
        return 1;
      }
    }
  }
  if (s_prev_dispatch) {
    return s_prev_dispatch(event, w);
  }
  return Fl::handle_(event, w);
}

// Keys no widget took, in any window: Escape and the panels' shortcuts
static int key_handler(int event) {
  if (event != FL_SHORTCUT) {
    return 0;
  }
  for (size_t i = 0; i < s_areas.size(); i++) {
    if (s_areas[i]->key_pressed()) {
      return 1;
    }
  }
  return 0;
}

/**
  Creates an area holding \p center, which takes whatever room docked panels
  leave it.  The area is not made current; it holds only the center and the
  panels docked in it.
*/
Fl_Dockable_Area::Fl_Dockable_Area(int X, int Y, int W, int H, Fl_Widget *center)
    : Fl_Tile(X, Y, W, H) {
  // Ended at once: the group made current before it is current again
  end();

  view_ = center;
  min_view_w_ = 300;
  min_view_h_ = 200;
  dock_band_ = 150;
  shrunk_ = false;
  factory_ = 0;
  factory_data_ = 0;
  callback_panel_ = 0;
  slid_ = 0;
  all_stripes_ = false;
  maximized_ = 0;
  captive_ = 0;
  place_holder_ = 0;
  tab_hint_ = 0;
  tab_with_ = 0;
  hint_kind_ = 0;
  place_side_ = FL_DOCKABLE_RIGHT;
  place_slot_ = 0;
  place_thick_ = 0;
  panel_menu_ = 0;
  menu_tracker_ = 0;
  menu_pending_ = false;
  cascade_ = 0;
  stripe_options_ = 0;
  add(center);
  resizable(center);
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    Fl_Group *cur = Fl_Group::current();
    Fl_Group::current(0);
    stripes_[s] = new Fl_Dockable_Stripe(this, s);
    Fl_Group::current(cur);
    add(stripes_[s]);
  }
  if (s_areas.empty()) {
    s_prev_dispatch = Fl::event_dispatch();
    Fl::event_dispatch(dispatch);
    Fl::add_handler(key_handler);
  }
  s_areas.push_back(this);
  when(FL_WHEN_RELEASE);
  relayout();
}

/**
  Adds a place named \p name docked along \p side at \p order among the
  places there, or returns the place with that name.  The area keeps it.
*/
Fl_Dockable_Place *Fl_Dockable_Area::add_place(const char *name, int side, double order) {
  Fl_Dockable_Place *p = find_place(name);
  if (p) {
    return p;
  }
  p = new Fl_Dockable_Place();
  if (name) {
    p->name = name;
  }
  if (side < 0 || side >= FL_DOCKABLE_NUM_SIDES) {
    side = FL_DOCKABLE_RIGHT;
  }
  p->side = side;
  p->order = order;
  places_.push_back(p);
  return p;
}

/** The place named \p name, or 0. */
Fl_Dockable_Place *Fl_Dockable_Area::find_place(const char *name) const {
  if (!name) {
    return 0;
  }
  for (size_t i = 0; i < places_.size(); i++) {
    if (places_[i]->name == name) {
      return places_[i];
    }
  }
  return 0;
}

Fl_Dockable_Place *Fl_Dockable_Area::place(int i) const {
  if (i < 0 || i >= (int)places_.size()) {
    return 0;
  }
  return places_[i];
}

Fl_Dockable_Panel *Fl_Dockable_Area::panel(int i) const {
  if (i < 0 || i >= (int)all_.size()) {
    return 0;
  }
  return all_[i];
}

/** The panel named \p name, or 0. */
Fl_Dockable_Panel *Fl_Dockable_Area::find(const char *name) const {
  if (!name) {
    return 0;
  }
  for (size_t i = 0; i < all_.size(); i++) {
    if (!strcmp(all_[i]->name(), name)) {
      return all_[i];
    }
  }
  return 0;
}

/**
  Shows the panel of a place: the one last shown there, or the first, or one
  the panel factory makes.  Returns 0 if the place has no panel.
*/
int Fl_Dockable_Area::show_place(Fl_Dockable_Place *p) {
  if (!p) {
    return 0;
  }
  Fl_Dockable_Panel *d = 0;
  if (p->last && p->last->place() == p) {
    d = p->last;
  }
  for (size_t i = 0; i < all_.size() && !d; i++) {
    if (all_[i]->place() == p) {
      d = all_[i];
    }
  }
  if (!d && factory_) {
    d = factory_(this, p, factory_data_);
  }
  if (!d) {
    return 0;
  }
  d->open_request();
  return 1;
}

void Fl_Dockable_Area::register_panel(Fl_Dockable_Panel *d) {
  all_.push_back(d);
  update_panel_menu();
}

// A panel deleted leaves the area's lists
void Fl_Dockable_Area::forget_panel(Fl_Dockable_Panel *d) {
  std::vector<Fl_Dockable_Panel *>::iterator it = std::find(all_.begin(), all_.end(), d);
  if (it != all_.end()) {
    all_.erase(it);
  }
  it = std::find(hidden_docked_.begin(), hidden_docked_.end(), d);
  if (it != hidden_docked_.end()) {
    hidden_docked_.erase(it);
  }
  for (size_t i = 0; i < places_.size(); i++) {
    if (places_[i]->last == d) {
      places_[i]->last = 0;
    }
  }
  if (slid_ == d) {
    slid_ = 0;
  }
  if (callback_panel_ == d) {
    callback_panel_ = 0;
  }
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    stripes_[s]->forget(d);
  }
  if (has_column(d)) {
    hide_column(d);
  }
  forget_hidden(d);
  update_panel_menu();
}

// Tells the application what the user did
void Fl_Dockable_Area::notify(Fl_Dockable_Panel *d, Fl_Callback_Reason reason) {
  redraw_stripes();
  update_panel_menu();
  callback_panel_ = d;
  if (reason == FL_REASON_UNKNOWN) {
    reason = FL_REASON_CHANGED;
  }
  do_callback(reason);
}

void Fl_Dockable_Area::captive_layer(Fl_Group *g) {
  captive_ = g;
  for (size_t i = 0; i < all_.size(); i++) {
    all_[i]->captive(g);
  }
}

/**
  Hides the docked panels shown, through their hide_request(), so the center
  has the whole area, or shows them again through their open_request().
*/
void Fl_Dockable_Area::hide_docked(int on) {
  if (on) {
    if (!hidden_docked_.empty()) {
      return;
    }
    for (size_t i = 0; i < all_.size(); i++) {
      if (all_[i]->docked() && all_[i]->panel_shown()) {
        hidden_docked_.push_back(all_[i]);
      }
    }
    std::vector<Fl_Dockable_Panel *> hide = hidden_docked_;
    for (size_t i = 0; i < hide.size(); i++) {
      hide[i]->hide_request();
    }
  } else {
    std::vector<Fl_Dockable_Panel *> shown = hidden_docked_;
    hidden_docked_.clear();
    for (size_t i = 0; i < shown.size(); i++) {
      shown[i]->open_request();
    }
  }
}

// One panel for each place with a button on a side's stripe, in the buttons' order: the panel
// shown there, or else the one last shown
std::vector<Fl_Dockable_Panel *> Fl_Dockable_Area::stripe_panels(int side) {
  std::vector<Fl_Dockable_Panel *> panels;
  for (size_t i = 0; i < all_.size(); i++) {
    Fl_Dockable_Place *p = all_[i]->place();
    if (p->stripe != side) {
      continue;
    }
    Fl_Dockable_Panel *d = all_[i];
    if (p->last && p->last->place() == p) {
      d = p->last;
    }
    for (size_t j = 0; j < all_.size(); j++) {
      if (all_[j]->place() == p && all_[j]->panel_shown()) {
        d = all_[j];
      }
    }
    if (std::find(panels.begin(), panels.end(), d) != panels.end()) {
      continue;
    }
    bool listed = false;
    for (size_t k = 0; k < panels.size(); k++) {
      if (panels[k]->place() == p) {
        listed = true;
      }
    }
    if (listed) {
      continue;
    }
    size_t k = 0;
    while (k < panels.size() && panels[k]->place()->stripe_order <= p->stripe_order) {
      k++;
    }
    panels.insert(panels.begin() + k, d);
  }
  return panels;
}

// The order that puts a button at the end of a side's stripe
double Fl_Dockable_Area::stripe_end(int side) {
  std::vector<Fl_Dockable_Panel *> panels = stripe_panels(side);
  if (panels.empty()) {
    return 0;
  }
  return panels.back()->place()->stripe_order + 10;
}

void Fl_Dockable_Area::show_all_stripes(bool on) {
  all_stripes_ = on;
  relayout();
}

int Fl_Dockable_Area::stripe_at(int X_root, int Y_root, int &X, int &Y) {
  Fl_Window *top = window();
  if (!top) {
    return -1;
  }
  X = X_root - top->x_root();
  Y = Y_root - top->y_root();
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    Fl_Dockable_Stripe *sp = stripes_[s];
    if (sp->visible() && X >= sp->x() && Y >= sp->y() && X < sp->x() + sp->w() &&
        Y < sp->y() + sp->h()) {
      return s;
    }
  }
  return -1;
}

// Over a stripe, the stripe is marked; over the area, the placeholder or the box shows where the
// panel would go
void Fl_Dockable_Area::drag_button_over(Fl_Dockable_Panel *d, int X_root, int Y_root) {
  if (!d) {
    remove_place_holder();
    remove_tab_hint();
    return;
  }
  int X, Y;
  int s = stripe_at(X_root, Y_root, X, Y);
  if (s >= 0) {
    show_stripe_hint(s);
    return;
  }
  int side, slot, kind;
  Fl_Widget *part;
  if (hit_test(X_root, Y_root, side, slot, part, kind) && (part || fits(d, side))) {
    if (part) {
      show_tab_hint(part, kind);
    } else {
      show_place_holder(side, slot, room_for(d, side));
    }
    return;
  }
  remove_place_holder();
  remove_tab_hint();
}

// The panel goes where a panel dragged there would: docked beside, as a tab or stacked, or
// floating at the point, captive over the area's window and in a window of its own elsewhere.  It no
// longer auto-hides; its button stays.
void Fl_Dockable_Area::drop_panel_at(Fl_Dockable_Panel *d, int X_root, int Y_root) {
  Fl_Dockable_Place *p = d->place();
  int side, slot, kind;
  Fl_Widget *part;
  bool dock = hit_test(X_root, Y_root, side, slot, part, kind) && (part || fits(d, side));
  if (part && fl_dockable_first_panel(part) == d) {
    // Dropped on itself
    return;
  }
  // One that cannot float stays where it is
  if (!dock && !(d->dock_flags() & FL_DOCKABLE_FLOATABLE)) {
    return;
  }
  if (dock) {
    if (part) {
      place_by(d, part, kind);
    } else {
      p->side = side;
      p->order = free_order(d, side, order_at(side, slot));
      p->sub = 0;
      p->cross = 0;
    }
  }
  if (d->panel_shown()) {
    d->hide_panel();
  }
  p->auto_hide = false;
  if (dock) {
    p->docked = true;
  } else {
    Fl_Window *top = window();
    bool captive = captive_ && top && X_root >= top->x_root() && Y_root >= top->y_root() &&
                   X_root < top->x_root() + top->w() && Y_root < top->y_root() + top->h();
    p->docked = false;
    p->float_set = true;
    p->float_captive = captive;
    p->float_x = X_root;
    p->float_y = Y_root;
    if (captive) {
      p->float_x -= top->x_root();
      p->float_y -= top->y_root();
    }
    // A window already made for it goes, so it is made again where it was dropped
    d->unfloat();
  }
  Fl_Widget_Tracker alive(d);
  d->open_request();
  relayout();
  Fl_Dockable_Panel *about = d;
  if (alive.deleted()) {
    about = 0;
  }
  notify(about, FL_REASON_DRAGGED);
}

// The place of a panel dropped on a docked part: a tab of it, stacked before or after it along its
// place, or before or after it across
void Fl_Dockable_Area::place_by(Fl_Dockable_Panel *d, Fl_Widget *part, int kind) {
  Fl_Dockable_Place *p = d->place();
  Fl_Dockable_Place *q = fl_dockable_place_of(part);
  p->side = side_of(part);
  p->order = q->order;
  p->width = q->width;
  p->height = q->height;
  p->sub = q->sub;
  p->extent = q->extent;
  p->cross = q->cross;
  p->cross_extent = q->cross_extent;
  if (kind == FL_DOCKABLE_HINT_BEFORE || kind == FL_DOCKABLE_HINT_AFTER) {
    p->sub = free_sub(d, p->side, p->order, sub_at(part, kind == FL_DOCKABLE_HINT_AFTER));
    p->extent = 0;
    p->cross = 0;
    p->cross_extent = 0;
  } else if (kind == FL_DOCKABLE_HINT_ACROSS_BEFORE || kind == FL_DOCKABLE_HINT_ACROSS_AFTER) {
    p->cross = free_cross(d, p->side, p->order, p->sub,
                          cross_at(part, kind == FL_DOCKABLE_HINT_ACROSS_AFTER));
    p->cross_extent = 0;
  }
}

// A box over a stripe a dragged panel would auto-hide on
void Fl_Dockable_Area::show_stripe_hint(int side) {
  remove_place_holder();
  if (tab_hint_ && hint_kind_ == FL_DOCKABLE_HINT_STRIPE && tab_with_ == stripes_[side]) {
    return;
  }
  remove_tab_hint();
  Fl_Dockable_Stripe *sp = stripes_[side];
  Fl_Group *cur = Fl_Group::current();
  Fl_Group::current(nullptr);
  tab_hint_ = new Fl_Box(FL_DOWN_BOX, sp->x(), sp->y(), sp->w(), sp->h(), nullptr);
  Fl_Group::current(cur);
  tab_hint_->color(FL_SELECTION_COLOR);
  tab_with_ = sp;
  hint_kind_ = FL_DOCKABLE_HINT_STRIPE;
  add(tab_hint_);
  redraw();
}

// A button moves; a panel slid out from it slides away first
void Fl_Dockable_Area::move_button(Fl_Dockable_Panel *d, int side, double order) {
  if (slid_ == d && d->place()->stripe != side) {
    d->hide_request();
  }
  d->place()->stripe = side;
  d->place()->stripe_order = order;
  relayout();
  notify(d, FL_REASON_DRAGGED);
}

void Fl_Dockable_Area::inner_rect(int &X, int &Y, int &W, int &H) {
  int t[FL_DOCKABLE_NUM_SIDES];
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    t[s] = 0;
    if (all_stripes_ || (stripe_options_ & FL_DOCKABLE_STRIPE_SHOW_EMPTY) || !stripe_panels(s).empty()) {
      t[s] = stripe_thickness();
    }
  }
  X = x() + t[FL_DOCKABLE_LEFT];
  Y = y() + t[FL_DOCKABLE_TOP];
  W = w() - t[FL_DOCKABLE_LEFT] - t[FL_DOCKABLE_RIGHT];
  H = h() - t[FL_DOCKABLE_TOP] - t[FL_DOCKABLE_BOTTOM];
}

// Slides out along the stripe's edge of the area inside the stripes: the full height at its width
// at the left and right, the full width at its height at the top and bottom
void Fl_Dockable_Area::slide_out(Fl_Dockable_Panel *d) {
  if (slid_ && slid_ != d) {
    slid_->hide_request();
  }
  int side = d->place()->stripe;
  int IX, IY, IW, IH;
  inner_rect(IX, IY, IW, IH);
  int X = IX;
  int Y = IY;
  int W = IW;
  int H = IH;
  if (fl_dockable_column_side(side)) {
    W = std::min(fl_dockable_want(d, side) + 4, IW * 4 / 5);
    if (side == FL_DOCKABLE_RIGHT) {
      X = IX + IW - W;
    }
  } else {
    H = std::min(fl_dockable_want(d, side) + 4, IH * 4 / 5);
    if (side == FL_DOCKABLE_BOTTOM) {
      Y = IY + IH - H;
    }
  }
  d->slide_out(X, Y, W, H);
  slid_ = d;
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    stripes_[s]->redraw();
  }
}

void Fl_Dockable_Area::slid_away(Fl_Dockable_Panel *d) {
  if (slid_ == d) {
    slid_ = nullptr;
  }
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    stripes_[s]->redraw();
  }
}

// A click outside the panel slid out and its button, but in a menu, a tooltip or a modal window,
// slides it away
void Fl_Dockable_Area::pushed_at(Fl_Window *w) {
  Fl_Dockable_Panel *d = slid_;
  if (!d || !d->window() || Fl::grab() || Fl::modal()) {
    return;
  }
  if (w && (w->menu_window() || w->tooltip_window())) {
    return;
  }
  Fl_Window *sw = d->window();
  int X = Fl::event_x_root();
  int Y = Fl::event_y_root();
  if (X >= sw->x_root() && Y >= sw->y_root() && X < sw->x_root() + sw->w() &&
      Y < sw->y_root() + sw->h()) {
    return;
  }
  Fl_Window *top = window();
  if (top) {
    int bx = X - top->x_root();
    int by = Y - top->y_root();
    for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
      if (stripes_[s]->visible() && stripes_[s]->button_at(bx, by) == d) {
        return;
      }
    }
  }
  d->hide_request();
}

// Panels docked in the area are deleted with it; a floating panel's window
// is the application's to delete, and the panel forgets the area.
Fl_Dockable_Area::~Fl_Dockable_Area() {
  Fl_Dockable_Group::check_origin_target(this);
  Fl::remove_timeout(menu_timeout_cb, this);
  if (menu_tracker_ && !menu_tracker_->deleted() && !panel_menu_path_.empty()) {
    int i = panel_menu_->find_index(panel_menu_path_.c_str());
    if (i >= 0) {
      panel_menu_->clear_submenu(i);
    }
  }
  delete menu_tracker_;
  menu_tracker_ = 0;
  panel_menu_ = 0;
  // Panels not docked here outlive the area, with a copy of their place
  for (size_t i = 0; i < all_.size(); i++) {
    Fl_Dockable_Panel *d = all_[i];
    Fl_Dockable_Place *copy = new Fl_Dockable_Place(*d->place_);
    copy->last = 0;
    d->place_ = copy;
    d->own_place_ = true;
    d->area_ = 0;
  }
  all_.clear();
  // The dispatch the areas put in front goes with the last of them
  std::vector<Fl_Dockable_Area *>::iterator it = std::find(s_areas.begin(), s_areas.end(), this);
  if (it != s_areas.end()) {
    s_areas.erase(it);
  }
  if (s_areas.empty()) {
    Fl::remove_handler(key_handler);
    if (Fl::event_dispatch() == dispatch) {
      Fl::event_dispatch(s_prev_dispatch);
    }
    s_prev_dispatch = 0;
  }
  for (size_t i = 0; i < places_.size(); i++) {
    delete places_[i];
  }
}

void Fl_Dockable_Area::redraw_stripes() {
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    stripes_[s]->redraw();
  }
}

// Hides or shows the subwindows of every panel in a place, as it moves to another group
void Fl_Dockable_Area::hide_subwindows_in(Fl_Widget *c) {
  std::vector<Fl_Dockable_Panel *> panels;
  fl_dockable_panels_in(c, panels);
  for (size_t i = 0; i < panels.size(); i++) {
    panels[i]->hide_subwindows();
  }
}

void Fl_Dockable_Area::show_subwindows_in(Fl_Widget *c) {
  std::vector<Fl_Dockable_Panel *> panels;
  fl_dockable_panels_in(c, panels);
  for (size_t i = 0; i < panels.size(); i++) {
    panels[i]->show_subwindows();
  }
}

// The size across its border a docked panel, or the placeholder, has
int Fl_Dockable_Area::thickness(Fl_Widget *c, int side) {
  if (c == place_holder_) {
    return place_thick_;
  }
  if (fl_dockable_column_side(side)) {
    return fl_dockable_place_of(c)->width;
  }
  return fl_dockable_place_of(c)->height;
}

// The room the panels docked along a side take, without the placeholder
int Fl_Dockable_Area::side_thickness(int side) {
  std::vector<int> th[FL_DOCKABLE_NUM_SIDES];
  int t[FL_DOCKABLE_NUM_SIDES];
  hit_thickness(th, t);
  return t[side];
}

// The sizes across their sides the places along each side are drawn at, without the placeholder:
// their own, or shrunk in proportion to fit, as relayout() does
void Fl_Dockable_Area::hit_thickness(std::vector<int> th[], int t[]) {
  for (int side = 0; side < FL_DOCKABLE_NUM_SIDES; side++) {
    t[side] = 0;
    th[side].clear();
    for (size_t i = 0; i < columns_[side].size(); i++) {
      if (columns_[side][i] != place_holder_) {
        th[side].push_back(thickness(columns_[side][i], side));
        t[side] += th[side].back();
      }
    }
  }
  int IX, IY, IW, IH;
  inner_rect(IX, IY, IW, IH);
  fit_thickness(th, t, FL_DOCKABLE_LEFT, FL_DOCKABLE_RIGHT, IW - 50);
  fit_thickness(th, t, FL_DOCKABLE_TOP, FL_DOCKABLE_BOTTOM, IH - 50);
}

// The side a docked entry, or the placeholder, is along, or -1
int Fl_Dockable_Area::side_of(Fl_Widget *c) {
  for (int side = 0; side < FL_DOCKABLE_NUM_SIDES; side++) {
    if (slot_of(c, side)) {
      return side;
    }
  }
  return -1;
}

// The place along a side that holds a docked panel, alone or as a tab, or 0
Fl_Widget *Fl_Dockable_Area::slot_of(Fl_Widget *c, int side) {
  for (size_t i = 0; i < columns_[side].size(); i++) {
    Fl_Widget *e = columns_[side][i];
    if (e == c) {
      return e;
    }
    if (c && (dynamic_cast<Fl_Dockable_Tabs *>(e) || dynamic_cast<Fl_Dockable_Stack *>(e)) &&
        e->as_group()->contains(c)) {
      return e;
    }
  }
  return nullptr;
}

// A place along the side with the panel's order, which it joins as a tab, or 0
Fl_Widget *Fl_Dockable_Area::same_order(Fl_Dockable_Panel *d, int side) {
  for (size_t i = 0; i < columns_[side].size(); i++) {
    Fl_Widget *e = columns_[side][i];
    if (e != place_holder_ && e != d && fl_dockable_place_of(e) != d->place() &&
        fl_dockable_place_of(e)->order == d->place()->order) {
      return e;
    }
  }
  return nullptr;
}


/**
  Lays out the stripes, the docked places and the center again.  The columns
  at the left and right take the area's height; the rows at the top and
  bottom, the center's width.  The center takes the room they leave.  An
  application calls it after changing the size of a docked place.
*/
void Fl_Dockable_Area::relayout() {
  // A maximized place takes the whole area, and the other places are hidden until it is not.  The
  // center is not hidden, as an OpenGL view would lose its state, but made a pixel outside the area, where
  // it is clipped away.
  if (maximized_ && side_of(maximized_) < 0) {
    maximized_ = nullptr;
  }
  if (!maximized_) {
    for (size_t i = 0; i < max_hidden_.size(); i++) {
      if (side_of(max_hidden_[i]) >= 0) {
        max_hidden_[i]->show();
      }
    }
    max_hidden_.clear();
  } else {
    for (int i = 0; i < children(); i++) {
      Fl_Widget *c = child(i);
      if (c != maximized_ && c != place_holder_ && c->visible() && side_of(c) >= 0) {
        c->hide();
        max_hidden_.push_back(c);
      }
    }
  }
  // The stripes with buttons take the area's edges, the left and right their full height
  int IX, IY, IW, IH;
  inner_rect(IX, IY, IW, IH);
  int st = stripe_thickness();
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    Fl_Dockable_Stripe *sp = stripes_[s];
    if (!all_stripes_ && !(stripe_options_ & FL_DOCKABLE_STRIPE_SHOW_EMPTY) && stripe_panels(s).empty()) {
      sp->resize(x(), y(), 0, 0);
      sp->hide();
      continue;
    }
    if (s == FL_DOCKABLE_LEFT) {
      sp->resize(x(), y(), st, h());
    } else if (s == FL_DOCKABLE_RIGHT) {
      sp->resize(x() + w() - st, y(), st, h());
    } else if (s == FL_DOCKABLE_TOP) {
      sp->resize(IX, y(), IW, st);
    } else {
      sp->resize(IX, y() + h() - st, IW, st);
    }
    sp->show();
    sp->redraw();
  }

  if (maximized_) {
    view_->resize(IX - 2, IY - 2, 1, 1);
    maximized_->resize(IX, IY, IW, IH);
    init_sizes();
    redraw();
    return;
  }

  int t[FL_DOCKABLE_NUM_SIDES];
  std::vector<int> th[FL_DOCKABLE_NUM_SIDES];
  for (int side = 0; side < FL_DOCKABLE_NUM_SIDES; side++) {
    t[side] = 0;
    for (size_t i = 0; i < columns_[side].size(); i++) {
      th[side].push_back(thickness(columns_[side][i], side));
      t[side] += th[side].back();
    }
  }
  // In a window too small for them, the docked panels shrink in proportion, so all stay in it
  // beside the center; their places keep their sizes for when there is room
  bool across = fit_thickness(th, t, FL_DOCKABLE_LEFT, FL_DOCKABLE_RIGHT, IW - 50);
  bool down = fit_thickness(th, t, FL_DOCKABLE_TOP, FL_DOCKABLE_BOTTOM, IH - 50);
  shrunk_ = across || down;
  int vw = IW - t[FL_DOCKABLE_LEFT] - t[FL_DOCKABLE_RIGHT];
  if (vw < 50) {
    vw = 50;
  }
  int vh = IH - t[FL_DOCKABLE_TOP] - t[FL_DOCKABLE_BOTTOM];
  if (vh < 50) {
    vh = 50;
  }
  int vx = IX + t[FL_DOCKABLE_LEFT];
  int vy = IY + t[FL_DOCKABLE_TOP];

  int X = IX;
  for (size_t i = 0; i < columns_[FL_DOCKABLE_LEFT].size(); i++) {
    Fl_Widget *c = columns_[FL_DOCKABLE_LEFT][i];
    int cw = th[FL_DOCKABLE_LEFT][i];
    c->resize(X, IY, cw, IH);
    X += cw;
  }
  X = vx + vw;
  for (size_t i = 0; i < columns_[FL_DOCKABLE_RIGHT].size(); i++) {
    Fl_Widget *c = columns_[FL_DOCKABLE_RIGHT][i];
    int cw = th[FL_DOCKABLE_RIGHT][i];
    c->resize(X, IY, cw, IH);
    X += cw;
  }
  int Y = IY;
  for (size_t i = 0; i < columns_[FL_DOCKABLE_TOP].size(); i++) {
    Fl_Widget *c = columns_[FL_DOCKABLE_TOP][i];
    int ch = th[FL_DOCKABLE_TOP][i];
    c->resize(vx, Y, vw, ch);
    Y += ch;
  }
  Y = vy + vh;
  for (size_t i = 0; i < columns_[FL_DOCKABLE_BOTTOM].size(); i++) {
    Fl_Widget *c = columns_[FL_DOCKABLE_BOTTOM][i];
    int ch = th[FL_DOCKABLE_BOTTOM][i];
    c->resize(vx, Y, vw, ch);
    Y += ch;
  }
  view_->resize(vx, vy, vw, vh);

  init_sizes();
  init_size_range(150, 50);
  size_range(view_, min_view_w_, min_view_h_);
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    if (fl_dockable_column_side(s)) {
      size_range(stripes_[s], st, 0, st, 0x7FFFFFFF);
    } else {
      size_range(stripes_[s], 0, st, 0x7FFFFFFF, st);
    }
  }
  redraw();
}

// The docked panels' sizes, as the user may have dragged their borders
void Fl_Dockable_Area::keep_widths() {
  // A maximized place's size, and the hidden places', are not the ones the user gave them
  if (maximized_ || !max_hidden_.empty()) {
    return;
  }
  // Nor are sizes shrunk to fit a small window
  if (shrunk_) {
    return;
  }
  for (int side = 0; side < FL_DOCKABLE_NUM_SIDES; side++) {
    for (size_t i = 0; i < columns_[side].size(); i++) {
      Fl_Widget *c = columns_[side][i];
      if (c == place_holder_) {
        continue;
      }
      // Every panel in a place takes its size, and every panel in a part of a stack that part's
      // size along it
      std::vector<Fl_Dockable_Panel *> panels;
      fl_dockable_panels_in(c, panels);
      for (size_t k = 0; k < panels.size(); k++) {
        if (fl_dockable_column_side(side)) {
          panels[k]->place()->width = c->w();
        } else {
          panels[k]->place()->height = c->h();
        }
      }
      keep_extents(c);
    }
  }
}

// The size along its stack of each part of a stack, and of each part of a stack within it
void Fl_Dockable_Area::keep_extents(Fl_Widget *c) {
  Fl_Dockable_Stack *stack = dynamic_cast<Fl_Dockable_Stack *>(c);
  for (int k = 0; stack && k < stack->parts(); k++) {
    Fl_Widget *part = stack->part(k);
    std::vector<Fl_Dockable_Panel *> in;
    fl_dockable_panels_in(part, in);
    int e = part->w();
    if (stack->is_vertical()) {
      e = part->h();
    }
    for (size_t j = 0; j < in.size(); j++) {
      if (stack->level() == 0) {
        in[j]->place()->extent = e;
      } else {
        in[j]->place()->cross_extent = e;
      }
    }
    keep_extents(part);
  }
}

/**
  Resizes the area and lays out its places again: the docked places keep
  their sizes and the center takes the change, until the window is too small
  for them, when they shrink in proportion.
*/
void Fl_Dockable_Area::resize(int X, int Y, int W, int H) {
  Fl_Widget::resize(X, Y, W, H);
  relayout();
}

int Fl_Dockable_Area::has_column(Fl_Dockable_Panel *d) {
  return side_of(d) >= 0;
}

/**
  Returns true if panel \p d fits docked along \p side at the size it wants,
  leaving the center its minimum size.  A panel joining the tabs of its own
  place always fits.  A panel shown where it does not fit floats instead.
*/
int Fl_Dockable_Area::fits(Fl_Dockable_Panel *d, int side) {
  // A panel joining tabs there always fits
  if (side == d->place()->side && same_order(d, side)) {
    return true;
  }
  return room_for(d, side) >= fl_dockable_want(d, side);
}

// Columns take width from the center, rows height.  A panel in the same place, which it would take
// over, leaves its room.
int Fl_Dockable_Area::room_for(Fl_Dockable_Panel *d, int side) {
  int others = 0;
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    if (fl_dockable_column_side(s) != fl_dockable_column_side(side)) {
      continue;
    }
    for (size_t i = 0; i < columns_[s].size(); i++) {
      Fl_Widget *c = columns_[s][i];
      if (c != d && c != place_holder_ && fl_dockable_place_of(c) != d->place()) {
        others += thickness(c, s);
      }
    }
  }
  int IX, IY, IW, IH;
  inner_rect(IX, IY, IW, IH);
  int room = IH - min_view_h_ - others;
  if (fl_dockable_column_side(side)) {
    room = IW - min_view_w_ - others;
  }
  return std::min(fl_dockable_want(d, side), room);
}

// Panels in one place take turns: the one shown takes over the place of another there
void Fl_Dockable_Area::show_column(Fl_Dockable_Panel *d) {
  d->settle();
  if (has_column(d)) {
    // Shown in a place maximize hid, it brings the places back
    if (maximized_ && slot_of(d, side_of(d)) != maximized_) {
      maximized_ = nullptr;
      relayout();
    }
    return;
  }
  maximized_ = nullptr;
  relayout();
  // Hidden by tabs or by maximize when it was last docked
  d->set_visible();
  Fl_Dockable_Panel *other = nullptr;
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES && !other; s++) {
    for (size_t k = 0; k < columns_[s].size() && !other; k++) {
      std::vector<Fl_Dockable_Panel *> panels;
      fl_dockable_panels_in(columns_[s][k], panels);
      for (size_t j = 0; j < panels.size() && !other; j++) {
        if (panels[j] != d && panels[j]->place() == d->place()) {
          other = panels[j];
        }
      }
    }
  }
  if (other) {
    other->hide_panel();
  }

  Fl_Dockable_Place *p = d->place();
  int side = p->side;

  // A tab, or a part of the stack, of the place with its order
  Fl_Widget *join = same_order(d, side);
  if (join) {
    add_to_place(join, d);
    return;
  }

  int thick = room_for(d, side);
  if (fl_dockable_column_side(side)) {
    p->width = thick;
  } else {
    p->height = thick;
  }

  // In order of their places, left to right or top to bottom
  std::vector<Fl_Widget *> &v = columns_[side];
  size_t i = 0;
  while (i < v.size() && (v[i] == place_holder_ || fl_dockable_place_of(v[i])->order <= p->order)) {
    i++;
  }
  v.insert(v.begin() + i, d);
  add(d);
  relayout();
  // Subwindows in it are shown again in this window
  d->show_subwindows();
}

// A panel joins a place: as a tab of the part with its place in the stack, or as a part of its
// own, stacked in order
void Fl_Dockable_Area::add_to_place(Fl_Widget *entry, Fl_Dockable_Panel *d) {
  maximized_ = 0;
  int side = side_of(entry);
  Fl_Dockable_Place *p = d->place();
  Fl_Dockable_Place *q = fl_dockable_place_of(entry);
  p->side = side;
  p->order = q->order;
  p->width = q->width;
  p->height = q->height;
  add_at(entry, d, 0);
  relayout();
  d->show_subwindows();
}

// The key of a place at a level of nesting: where it goes along the place, or across it
static double key_at(Fl_Dockable_Place *p, int level) {
  if (level == 0) {
    return p->sub;
  }
  return p->cross;
}

// A panel goes into a node of a place at a level: into the part of a stack at that level with its
// key, a level down, or as a tab at the last level; else between the parts by its key, in a stack
// made at that level if there is none
void Fl_Dockable_Area::add_at(Fl_Widget *node, Fl_Dockable_Panel *d, int level) {
  int side = side_of(node);
  Fl_Dockable_Place *p = d->place();
  double key = key_at(p, level);
  Fl_Dockable_Stack *stack = dynamic_cast<Fl_Dockable_Stack *>(node);
  if (stack && stack->level() == level) {
    int before = 0;
    for (int i = 0; i < stack->parts(); i++) {
      Fl_Widget *part = stack->part(i);
      double k = key_at(fl_dockable_place_of(part), level);
      if (k == key) {
        if (level == 0) {
          add_at(part, d, 1);
        } else {
          join_tabs(part, d);
        }
        return;
      }
      if (k < key) {
        before++;
      }
    }
    stack->insert_part(d, before);
    return;
  }
  if (key_at(fl_dockable_place_of(node), level) == key) {
    if (level == 0) {
      add_at(node, d, 1);
    } else {
      join_tabs(node, d);
    }
    return;
  }
  // A stack at this level, in the node's place, holding it and the panel
  bool vertical = fl_dockable_column_side(side);
  if (level == 1) {
    vertical = !vertical;
  }
  hide_subwindows_in(node);
  Fl_Group *cur = Fl_Group::current();
  Fl_Group::current(0);
  stack = new Fl_Dockable_Stack(this, vertical, node->x(), node->y(), node->w(), node->h());
  stack->level(level);
  Fl_Group::current(cur);
  replace_in(node, stack);
  stack->insert_part(node, 0);
  show_subwindows_in(node);
  int at = 0;
  if (key_at(fl_dockable_place_of(node), level) < key) {
    at = 1;
  }
  stack->insert_part(d, at);
}

// Puts a place's new group where the old one was: among the area's places, or the parts of a stack
void Fl_Dockable_Area::replace_in(Fl_Widget *old, Fl_Widget *now) {
  Fl_Dockable_Stack *stack = dynamic_cast<Fl_Dockable_Stack *>(old->parent());
  if (stack) {
    int k = stack->find(old);
    stack->remove(old);
    stack->insert(*now, k);
    now->resize(old->x(), old->y(), old->w(), old->h());
    stack->init_sizes();
    return;
  }
  int side = side_of(old);
  std::vector<Fl_Widget *> &v = columns_[side];
  size_t i = std::find(v.begin(), v.end(), old) - v.begin();
  v[i] = now;
  remove(old);
  add(now);
  now->resize(old->x(), old->y(), old->w(), old->h());
}

// Takes a place, or a part of a stack, out of the area
void Fl_Dockable_Area::forget_hidden(Fl_Widget *c) {
  std::vector<Fl_Widget *>::iterator it = std::find(max_hidden_.begin(), max_hidden_.end(), c);
  if (it != max_hidden_.end()) {
    max_hidden_.erase(it);
  }
}

void Fl_Dockable_Area::remove_entry(Fl_Widget *c) {
  forget_hidden(c);
  Fl_Dockable_Stack *stack = dynamic_cast<Fl_Dockable_Stack *>(c->parent());
  if (stack) {
    stack->remove(c);
    Fl::delete_widget(c);
    unstacked(stack);
    return;
  }
  int side = side_of(c);
  if (side >= 0) {
    std::vector<Fl_Widget *> &v = columns_[side];
    v.erase(std::find(v.begin(), v.end(), c));
  }
  remove(c);
  Fl::delete_widget(c);
}

void Fl_Dockable_Area::hide_column(Fl_Dockable_Panel *d) {
  int side = side_of(d);
  if (side < 0) {
    return;
  }
  if (maximized_) {
    maximized_ = nullptr;
    relayout();
  }
  keep_widths();
  forget_hidden(d);
  Fl_Dockable_Tabs *tabs = dynamic_cast<Fl_Dockable_Tabs *>(d->parent());
  if (tabs) {
    tabs->remove(d);
    d->show_bar(true);
    untabbed(tabs);
    return;
  }
  Fl_Dockable_Stack *stack = dynamic_cast<Fl_Dockable_Stack *>(d->parent());
  if (stack) {
    stack->remove(d);
    unstacked(stack);
    return;
  }
  columns_[side].erase(std::find(columns_[side].begin(), columns_[side].end(), d));
  remove(d);
  relayout();
}

// A docked panel and the one joining it become tabs of one place, or of one part of a stack, at
// its size
void Fl_Dockable_Area::join_tabs(Fl_Widget *part, Fl_Dockable_Panel *d) {
  maximized_ = nullptr;
  Fl_Dockable_Tabs *tabs = dynamic_cast<Fl_Dockable_Tabs *>(part);
  if (!tabs) {
    Fl_Dockable_Panel *first = (Fl_Dockable_Panel *)part;
    first->hide_subwindows();
    Fl_Group *cur = Fl_Group::current();
    Fl_Group::current(nullptr);
    tabs = new Fl_Dockable_Tabs(this, part->x(), part->y(), part->w(), part->h());
    Fl_Group::current(cur);
    replace_in(first, tabs);
    tabs->add_panel(first);
    first->show_subwindows();
  }
  Fl_Dockable_Place *p = d->place();
  Fl_Dockable_Place *q = fl_dockable_place_of(tabs);
  p->side = side_of(tabs);
  p->order = q->order;
  p->width = q->width;
  p->height = q->height;
  p->sub = q->sub;
  p->extent = q->extent;
  p->cross = q->cross;
  p->cross_extent = q->cross_extent;
  tabs->add_panel(d);
  relayout();
  d->show_subwindows();
}

/**
  Gives the docked place of panel \p d the whole area, hiding the other
  places, or, with \p d 0, puts every place back at the size it had.  A panel
  not docked is not maximized.  The center is not hidden but moved just
  outside the area, so an OpenGL view keeps its context.
*/
void Fl_Dockable_Area::maximize(Fl_Dockable_Panel *d) {
  // The sizes the user gave the places are kept before one takes the whole area
  if (!maximized_) {
    keep_widths();
  }
  maximized_ = nullptr;
  if (d) {
    int side = side_of(d);
    if (side >= 0) {
      maximized_ = slot_of(d, side);
    }
  }
  relayout();
}

/** Returns true if the place of panel \p d is maximized. */
int Fl_Dockable_Area::maximized(Fl_Dockable_Panel *d) {
  if (!maximized_) {
    return false;
  }
  int side = side_of(d);
  return side >= 0 && slot_of(d, side) == maximized_;
}

// Tabs left with one panel show it alone in their place; with none, the place closes
void Fl_Dockable_Area::untabbed(Fl_Dockable_Tabs *tabs) {
  maximized_ = nullptr;
  if (side_of(tabs) < 0) {
    return;
  }
  int n = tabs->panels();
  if (n == 0) {
    remove_entry(tabs);
  } else if (n == 1) {
    Fl_Dockable_Panel *r = tabs->panel(0);
    r->hide_subwindows();
    tabs->remove(r);
    r->show_bar(true);
    replace_in(tabs, r);
    Fl::delete_widget(tabs);
    if (!r->visible()) {
      r->show();
    }
    r->show_subwindows();
  } else if (!dynamic_cast<Fl_Dockable_Panel *>(tabs->value())) {
    tabs->value(tabs->panel(0));
  }
  relayout();
}

// A stack left with one part shows it alone in the place; with none, the place closes
void Fl_Dockable_Area::unstacked(Fl_Dockable_Stack *stack) {
  maximized_ = nullptr;
  if (side_of(stack) < 0) {
    return;
  }
  int n = stack->parts();
  if (n == 0) {
    remove_entry(stack);
  } else if (n == 1) {
    Fl_Widget *r = stack->part(0);
    hide_subwindows_in(r);
    stack->remove(r);
    replace_in(stack, r);
    Fl::delete_widget(stack);
    show_subwindows_in(r);
  } else {
    stack->layout_parts();
  }
  relayout();
}

// The slot along a side a position falls in: before the first docked panel whose middle is past
// it, counting from where the side's panels start, without the placeholder The place the position
// is over is given, and whether it is in its outer quarters across the side, where a panel goes
// beside it rather than into it.
int Fl_Dockable_Area::slot_along(int side, int pos, int start, Fl_Widget *&over, bool &edge) {
  over = nullptr;
  edge = true;
  int slot = 0;
  std::vector<int> th[FL_DOCKABLE_NUM_SIDES];
  int ts[FL_DOCKABLE_NUM_SIDES];
  hit_thickness(th, ts);
  for (size_t i = 0; i < columns_[side].size(); i++) {
    Fl_Widget *c = columns_[side][i];
    if (c == place_holder_) {
      continue;
    }
    int t = th[side][slot];
    if (pos < start + t) {
      over = c;
      edge = (pos < start + t / 8 || pos >= start + t - t / 8);
      if (pos < start + t / 2) {
        return slot;
      }
      return slot + 1;
    }
    start += t;
    slot++;
  }
  return slot;
}

// Over a place, away from its edges across the side: in the outer quarters of a part along the
// place it stacks before or after it; in the outer quarters across a part of a stack, it stacks
// across beside it; otherwise it joins it as a tab
static Fl_Widget *part_at(Fl_Widget *entry, bool vertical, int X, int Y, int &kind) {
  // The part along the place, then the part across it
  Fl_Widget *part = entry;
  Fl_Dockable_Stack *stack = dynamic_cast<Fl_Dockable_Stack *>(entry);
  if (stack && stack->level() == 0) {
    for (int i = 0; i < stack->parts(); i++) {
      Fl_Widget *p = stack->part(i);
      if (X >= p->x() && Y >= p->y() && X < p->x() + p->w() && Y < p->y() + p->h()) {
        part = p;
      }
    }
  } else {
    stack = 0;
  }
  Fl_Widget *leaf = part;
  Fl_Dockable_Stack *across = dynamic_cast<Fl_Dockable_Stack *>(part);
  for (int i = 0; across && i < across->parts(); i++) {
    Fl_Widget *p = across->part(i);
    if (X >= p->x() && Y >= p->y() && X < p->x() + p->w() && Y < p->y() + p->h()) {
      leaf = p;
    }
  }
  double f = (double)(X - part->x()) / part->w();
  double g = (double)(Y - leaf->y()) / leaf->h();
  if (vertical) {
    f = (double)(Y - part->y()) / part->h();
    g = (double)(X - leaf->x()) / leaf->w();
  }
  kind = FL_DOCKABLE_HINT_TAB;
  if (f < 0.25) {
    kind = FL_DOCKABLE_HINT_BEFORE;
    return part;
  }
  if (f >= 0.75) {
    kind = FL_DOCKABLE_HINT_AFTER;
    return part;
  }
  // Across a part of a stack, where a single place's own edges dock beside it instead
  if (stack || across) {
    if (g < 0.25) {
      kind = FL_DOCKABLE_HINT_ACROSS_BEFORE;
    } else if (g >= 0.75) {
      kind = FL_DOCKABLE_HINT_ACROSS_AFTER;
    }
  }
  return leaf;
}

// The side and slot a panel dropped at X, Y, in screen coordinates, goes in.  Over the docked
// panels it goes among them; over the center, within a band of a border, it goes along the nearest,
// next to the center.  Elsewhere it floats. Over a place, it goes into it, given as the part it joins
// or stacks by.
bool Fl_Dockable_Area::hit_test(int X_root, int Y_root, int &side, int &slot, Fl_Widget *&part,
                                int &kind) {
  part = nullptr;
  kind = 0;
  int ox, oy;
  Fl_Window *top = top_window_offset(ox, oy);
  int IX, IY, IW, IH;
  inner_rect(IX, IY, IW, IH);
  int rx = X_root - top->x() - ox - (IX - x());
  int ry = Y_root - top->y() - oy - (IY - y());
  if (rx < 0 || ry < 0 || rx >= IW || ry >= IH) {
    return false;
  }

  int tl = side_thickness(FL_DOCKABLE_LEFT);
  int tr = side_thickness(FL_DOCKABLE_RIGHT);
  int tt = side_thickness(FL_DOCKABLE_TOP);
  int tb = side_thickness(FL_DOCKABLE_BOTTOM);
  Fl_Widget *over = nullptr;
  bool edge = true;
  side = -1;
  if (rx < tl) {
    side = FL_DOCKABLE_LEFT;
    slot = slot_along(side, rx, 0, over, edge);
  } else if (rx >= IW - tr) {
    side = FL_DOCKABLE_RIGHT;
    slot = slot_along(side, rx, IW - tr, over, edge);
  } else if (ry < tt) {
    side = FL_DOCKABLE_TOP;
    slot = slot_along(side, ry, 0, over, edge);
  } else if (ry >= IH - tb) {
    side = FL_DOCKABLE_BOTTOM;
    slot = slot_along(side, ry, IH - tb, over, edge);
  }
  if (side >= 0) {
    if (over && !edge) {
      part = part_at(over, fl_dockable_column_side(side), IX + rx, IY + ry, kind);
    }
    return true;
  }

  int dist[FL_DOCKABLE_NUM_SIDES];
  dist[FL_DOCKABLE_LEFT] = rx - tl;
  dist[FL_DOCKABLE_RIGHT] = IW - tr - 1 - rx;
  dist[FL_DOCKABLE_TOP] = ry - tt;
  dist[FL_DOCKABLE_BOTTOM] = IH - tb - 1 - ry;
  side = -1;
  int best = dock_band();
  for (int s = 0; s < FL_DOCKABLE_NUM_SIDES; s++) {
    if (dist[s] < best) {
      best = dist[s];
      side = s;
    }
  }
  if (side < 0) {
    return false;
  }
  int n = 0;
  for (size_t i = 0; i < columns_[side].size(); i++) {
    if (columns_[side][i] != place_holder_) {
      n++;
    }
  }
  slot = 0;
  if (side == FL_DOCKABLE_LEFT || side == FL_DOCKABLE_TOP) {
    slot = n;
  }
  return true;
}

// The order a panel in a slot along a side takes, between its neighbors'
double Fl_Dockable_Area::order_at(int side, int slot) {
  std::vector<double> orders;
  for (size_t i = 0; i < columns_[side].size(); i++) {
    if (columns_[side][i] != place_holder_) {
      orders.push_back(fl_dockable_place_of(columns_[side][i])->order);
    }
  }
  if (orders.empty()) {
    return 0;
  }
  if (slot <= 0) {
    return orders[0] - 10;
  }
  if (slot >= (int)orders.size()) {
    return orders.back() + 10;
  }
  return 0.5 * (orders[slot - 1] + orders[slot]);
}

// The center is at the inner end of each side: after the last at the left and top, before the first
// at the right and bottom
double Fl_Dockable_Area::order_next_to_view(int side, Fl_Dockable_Panel *d) {
  int n = 0;
  double lo = 0;
  double hi = 0;
  for (size_t i = 0; i < columns_[side].size(); i++) {
    Fl_Widget *c = columns_[side][i];
    if (c == place_holder_ || c == d) {
      continue;
    }
    double o = fl_dockable_place_of(c)->order;
    if (n == 0 || o < lo) {
      lo = o;
    }
    if (n == 0 || o > hi) {
      hi = o;
    }
    n++;
  }
  if (n == 0) {
    return d->place()->order;
  }
  if (side == FL_DOCKABLE_LEFT || side == FL_DOCKABLE_TOP) {
    return hi + 10;
  }
  return lo - 10;
}

// A box over the docked panel a dragged one would join as a tab, kind 0, or over the half of it it
// would stack in, before it, kind 1, or after it, kind 2
void Fl_Dockable_Area::show_tab_hint(Fl_Widget *part, int kind) {
  remove_place_holder();
  if (tab_hint_ && tab_with_ == part && hint_kind_ == kind) {
    return;
  }
  remove_tab_hint();
  int X = part->x();
  int Y = part->y();
  int W = part->w();
  int H = part->h();
  const char *label = "Add as a tab";
  if (kind != FL_DOCKABLE_HINT_TAB) {
    label = "Dock here";
    bool vertical = fl_dockable_column_side(side_of(part));
    if (kind == FL_DOCKABLE_HINT_ACROSS_BEFORE || kind == FL_DOCKABLE_HINT_ACROSS_AFTER) {
      vertical = !vertical;
    }
    bool after = (kind == FL_DOCKABLE_HINT_AFTER || kind == FL_DOCKABLE_HINT_ACROSS_AFTER);
    if (vertical) {
      H /= 2;
      if (after) {
        Y += H;
      }
    } else {
      W /= 2;
      if (after) {
        X += W;
      }
    }
  }
  Fl_Group *cur = Fl_Group::current();
  Fl_Group::current(nullptr);
  tab_hint_ = new Fl_Box(FL_DOWN_BOX, X, Y, W, H, label);
  Fl_Group::current(cur);
  tab_hint_->color(FL_SELECTION_COLOR);
  tab_hint_->labelcolor(fl_contrast(FL_WHITE, FL_SELECTION_COLOR));
  tab_with_ = part;
  hint_kind_ = kind;
  add(tab_hint_);
  redraw();
}

// The place in its stack a panel stacked before or after a part takes, between its neighbors'
// An order no other place along the side has, docked or not, so a panel given it does not join a
// hidden panel's place when that is shown
double Fl_Dockable_Area::free_order(Fl_Dockable_Panel *d, int side, double order) {
  for (int n = 0; n < 100000; n++) {
    bool taken = false;
    for (size_t i = 0; i < places_.size() && !taken; i++) {
      Fl_Dockable_Place *p = places_[i];
      taken = (p != d->place() && p->side == side && p->order == order);
    }
    if (!taken) {
      break;
    }
    order += free_step(order);
  }
  return order;
}

// A place in a stack no other place in it has, docked or not
double Fl_Dockable_Area::free_sub(Fl_Dockable_Panel *d, int side, double order, double sub) {
  for (int n = 0; n < 100000; n++) {
    bool taken = false;
    for (size_t i = 0; i < places_.size() && !taken; i++) {
      Fl_Dockable_Place *p = places_[i];
      taken = (p != d->place() && p->side == side && p->order == order && p->sub == sub);
    }
    if (!taken) {
      break;
    }
    sub += free_step(sub);
  }
  return sub;
}

// A place across a part of a stack no other place there has, docked or not
double Fl_Dockable_Area::free_cross(Fl_Dockable_Panel *d, int side, double order, double sub,
                                    double cross) {
  for (int n = 0; n < 100000; n++) {
    bool taken = false;
    for (size_t i = 0; i < places_.size() && !taken; i++) {
      Fl_Dockable_Place *p = places_[i];
      taken = (p != d->place() && p->side == side && p->order == order && p->sub == sub &&
               p->cross == cross);
    }
    if (!taken) {
      break;
    }
    cross += free_step(cross);
  }
  return cross;
}

// A step that moves a value however large it is
double Fl_Dockable_Area::free_step(double v) {
  double step = fabs(v) * 1e-9;
  if (step < 0.01) {
    step = 0.01;
  }
  return step;
}

// The place across a part of a stack a panel stacked before or after it takes, between its
// neighbors'
double Fl_Dockable_Area::cross_at(Fl_Widget *part, bool after) {
  std::vector<double> crosses;
  Fl_Dockable_Stack *stack = dynamic_cast<Fl_Dockable_Stack *>(part->parent());
  if (stack && stack->level() != 1) {
    stack = 0;
  }
  for (int i = 0; stack && i < stack->parts(); i++) {
    crosses.push_back(fl_dockable_place_of(stack->part(i))->cross);
  }
  double cross = fl_dockable_place_of(part)->cross;
  if (!stack) {
    crosses.push_back(cross);
  }
  size_t k = std::find(crosses.begin(), crosses.end(), cross) - crosses.begin();
  if (after) {
    if (k + 1 < crosses.size()) {
      return 0.5 * (cross + crosses[k + 1]);
    }
    return cross + 10;
  }
  if (k > 0) {
    return 0.5 * (crosses[k - 1] + cross);
  }
  return cross - 10;
}

double Fl_Dockable_Area::sub_at(Fl_Widget *part, bool after) {
  std::vector<double> subs;
  Fl_Dockable_Stack *stack = dynamic_cast<Fl_Dockable_Stack *>(part->parent());
  if (stack && stack->level() != 0) {
    stack = 0;
  }
  for (int i = 0; stack && i < stack->parts(); i++) {
    subs.push_back(fl_dockable_place_of(stack->part(i))->sub);
  }
  double sub = fl_dockable_place_of(part)->sub;
  if (!stack) {
    subs.push_back(sub);
  }
  size_t k = std::find(subs.begin(), subs.end(), sub) - subs.begin();
  if (after) {
    if (k + 1 < subs.size()) {
      return 0.5 * (sub + subs[k + 1]);
    }
    return sub + 10;
  }
  if (k > 0) {
    return 0.5 * (subs[k - 1] + sub);
  }
  return sub - 10;
}

void Fl_Dockable_Area::remove_tab_hint() {
  if (!tab_hint_) {
    return;
  }
  remove(tab_hint_);
  Fl::delete_widget(tab_hint_);
  tab_hint_ = nullptr;
  tab_with_ = nullptr;
  redraw();
}

void Fl_Dockable_Area::show_place_holder(int side, int slot, int thick) {
  remove_tab_hint();
  if (place_holder_ && side == place_side_ && slot == place_slot_ && thick == place_thick_) {
    return;
  }
  remove_place_holder();
  Fl_Group *cur = Fl_Group::current();
  Fl_Group::current(nullptr);
  place_holder_ = new Fl_Box(FL_DOWN_BOX, 0, 0, 10, 10, "Dock here");
  Fl_Group::current(cur);
  place_holder_->color(FL_SELECTION_COLOR);
  place_holder_->labelcolor(fl_contrast(FL_WHITE, FL_SELECTION_COLOR));
  place_side_ = side;
  place_slot_ = slot;
  place_thick_ = thick;

  std::vector<Fl_Widget *> &v = columns_[side];
  size_t i = slot;
  if (i > v.size()) {
    i = v.size();
  }
  v.insert(v.begin() + i, place_holder_);
  add(place_holder_);
  relayout();
}

void Fl_Dockable_Area::remove_place_holder() {
  if (!place_holder_) {
    return;
  }
  int side = side_of(place_holder_);
  if (side >= 0) {
    columns_[side].erase(std::find(columns_[side].begin(), columns_[side].end(), place_holder_));
  }
  forget_hidden(place_holder_);
  remove(place_holder_);
  Fl::delete_widget(place_holder_);
  place_holder_ = nullptr;
  relayout();
}

/**
  Takes the docking events of a panel dragged over the area: a placeholder or
  a hint shows where it will dock, or a stripe where its button will go, and
  releasing it docks it there.  Other events go to the tile, so the borders
  between the center and the docked panels can be dragged.
*/
int Fl_Dockable_Area::handle(int event) {
  if (event == FL_DOCK_ENTER || event == FL_DOCK_DRAG || event == FL_DOCK_LEAVE ||
      event == FL_DOCK_RELEASE || event == FL_UNDOCK) {
    bool processed = false;
    int r = Fl_Dockable_Group::handle_docking_target(
        processed, this, event, inside_cb, acceptable_cb, state_cb, payload_cb, undock_cb);
    if ((event == FL_DOCK_ENTER || event == FL_DOCK_DRAG) && r) {
      Fl_Dockable_Panel *d =
          dynamic_cast<Fl_Dockable_Panel *>(Fl_Dockable_Group::active_dockable());
      int side, slot, kind;
      Fl_Widget *part;
      int X, Y;
      int s = stripe_at(Fl::event_x_root(), Fl::event_y_root(), X, Y);
      if (d && d->state() == Fl_Dockable_Group::DOCK && s >= 0) {
        show_stripe_hint(s);
      } else if (d && d->state() == Fl_Dockable_Group::DOCK &&
                 hit_test(Fl::event_x_root(), Fl::event_y_root(), side, slot, part, kind)) {
        if (part) {
          show_tab_hint(part, kind);
        } else {
          show_place_holder(side, slot, room_for(d, side));
        }
      }
    }
    if (processed) {
      return r;
    }
  }

  int r = Fl_Tile::handle(event);
  // A border between docked panels may have been dragged
  if (event == FL_RELEASE) {
    borders_dragged();
  }
  return r;
}

// The sizes the user dragged the borders to are kept, and saved
void Fl_Dockable_Area::borders_dragged() {
  keep_widths();
  notify(0, FL_REASON_CHANGED);
}

// Over the docked panels, or a band of the center along a border, where the panel fits; dropped
// elsewhere over the center, or with Ctrl held, a panel floats
bool Fl_Dockable_Area::inside_cb(Fl_Widget *target) {
  Fl_Dockable_Area *area = (Fl_Dockable_Area *)target;
  if (!Fl_Dockable_Group::is_dockable_inside(target) || Fl::event_state(FL_CTRL)) {
    return false;
  }
  Fl_Dockable_Panel *d = dynamic_cast<Fl_Dockable_Panel *>(Fl_Dockable_Group::active_dockable());
  int side, slot, kind;
  Fl_Widget *part;
  int X, Y;
  if (d && area->captive_ && area->stripe_at(Fl::event_x_root(), Fl::event_y_root(), X, Y) >= 0) {
    return true;
  }
  if (!d || !area->hit_test(Fl::event_x_root(), Fl::event_y_root(), side, slot, part, kind)) {
    return false;
  }
  return part || area->fits(d, side);
}

bool Fl_Dockable_Area::acceptable_cb(Fl_Widget *target, Fl_Dockable_Group *dockable) {
  return dynamic_cast<Fl_Dockable_Panel *>(dockable) != nullptr;
}

void Fl_Dockable_Area::state_cb(Fl_Widget *target, bool can_dock) {
  if (!can_dock) {
    ((Fl_Dockable_Area *)target)->remove_place_holder();
    ((Fl_Dockable_Area *)target)->remove_tab_hint();
  }
}

void Fl_Dockable_Area::payload_cb(Fl_Widget *target, Fl_Dockable_Group *dockable) {
  Fl_Dockable_Area *area = (Fl_Dockable_Area *)target;
  Fl_Dockable_Panel *d = dynamic_cast<Fl_Dockable_Panel *>(dockable);
  int side = area->place_side_;
  int slot = area->place_slot_;
  bool hit = (area->place_holder_ != nullptr);
  Fl_Widget *part = area->tab_with_;
  int kind = area->hint_kind_;
  if (part && kind == FL_DOCKABLE_HINT_STRIPE && d) {
    // Dropped on a stripe: its button goes there, and it auto-hides
    Fl_Dockable_Stripe *sp = (Fl_Dockable_Stripe *)part;
    int X = 0;
    int Y = 0;
    int s = area->stripe_at(Fl::event_x_root(), Fl::event_y_root(), X, Y);
    int stripe = FL_DOCKABLE_RIGHT;
    for (int k = 0; k < FL_DOCKABLE_NUM_SIDES; k++) {
      if (area->stripes_[k] == sp) {
        stripe = k;
      }
    }
    double order = area->stripe_end(stripe);
    if (s == stripe) {
      order = sp->order_at(X, Y);
    }
    area->remove_tab_hint();
    // Pinned, it docks where its place says
    d->place()->stripe = stripe;
    d->place()->stripe_order = order;
    d->place()->auto_hide = true;
    d->place()->docked = true;
    area->relayout();
    area->notify(d, FL_REASON_DRAGGED);
    return;
  }
  if (part && d) {
    // The place the hint shows, taken before the hint goes
    area->place_by(d, part, kind);
  }
  area->remove_place_holder();
  area->remove_tab_hint();
  if (!d) {
    return;
  }
  if (part && area->side_of(part) >= 0) {
    // A tab of the panel it was dropped on, or stacked by it
    d->place()->docked = true;
    if (kind == FL_DOCKABLE_HINT_TAB) {
      area->join_tabs(part, d);
    } else {
      area->add_to_place(area->slot_of(part, area->side_of(part)), d);
    }
    area->notify(d, FL_REASON_DRAGGED);
    return;
  }
  int kind2;
  if (!hit && !area->hit_test(Fl::event_x_root(), Fl::event_y_root(), side, slot, part, kind2)) {
    side = d->place()->side;
    slot = 0;
  }
  Fl_Dockable_Place *place = d->place();
  int thick = area->room_for(d, side);
  place->side = side;
  place->order = area->free_order(d, side, area->order_at(side, slot));
  place->sub = 0;
  place->cross = 0;
  if (fl_dockable_column_side(side)) {
    place->width = thick;
  } else {
    place->height = thick;
  }
  place->docked = true;
  area->show_column(d);
  area->notify(d, FL_REASON_DRAGGED);
}

void Fl_Dockable_Area::undock_cb(Fl_Widget *target, Fl_Dockable_Group *dockable) {
  Fl_Dockable_Area *area = (Fl_Dockable_Area *)target;
  area->maximized_ = nullptr;

  // FLTK leaves a Docking_Target_Box where the group was; the docked panels close up instead
  if (area->children() > 0) {
    Fl_Widget *c = area->child(area->children() - 1);
    if (c != area->view_ && c != area->place_holder_ && c != area->tab_hint_ &&
        area->side_of(c) < 0) {
      area->remove(c);
      Fl::delete_widget(c);
    }
  }

  int side = area->side_of(dockable);
  if (side >= 0) {
    area->keep_widths();
    std::vector<Fl_Widget *> &v = area->columns_[side];
    v.erase(std::find(v.begin(), v.end(), dockable));
  }
  // The layout is saved when the drag ends, with the panel's window where it was left
  area->relayout();
}
