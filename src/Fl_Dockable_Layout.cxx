//
// Layouts, menus and keys for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
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
#include "Fl_Dockable_Private.H"
#include <FL/Fl.H>
#include <FL/Fl_Menu_.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Window.H>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <string>

// A name as a preferences group: letters and digits as they are, anything else as _ and two hex
// digits, so different names stay different
static std::string group_name(const std::string &name) {
  std::string n;
  for (size_t i = 0; i < name.size(); i++) {
    unsigned char c = (unsigned char)name[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
      n += (char)c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "_%02X", c);
      n += buf;
    }
  }
  if (n.empty()) {
    n = "_";
  }
  return n;
}

// A position kept: finite and not so large that steps of it are lost, else the place's own
static void check_position(double &v, double def) {
  if (!(v == v) || v > 1e9 || v < -1e9) {
    v = def;
  }
}

// Values out of range, from another version or a hand edit, keep the place's
static void check_place(Fl_Dockable_Place *p, const Fl_Dockable_Place &def) {
  check_position(p->order, def.order);
  check_position(p->sub, def.sub);
  check_position(p->cross, def.cross);
  check_position(p->stripe_order, def.stripe_order);
  if (p->side < 0 || p->side >= FL_DOCKABLE_NUM_SIDES) {
    p->side = def.side;
  }
  if (p->stripe < -1 || p->stripe >= FL_DOCKABLE_NUM_SIDES) {
    p->stripe = def.stripe;
  }
  if (p->stripe < 0) {
    p->auto_hide = false;
  }
  if (p->width < 0) {
    p->width = def.width;
  }
  if (p->height < 0) {
    p->height = def.height;
  }
  if (p->extent < 0) {
    p->extent = def.extent;
  }
  if (p->cross_extent < 0) {
    p->cross_extent = def.cross_extent;
  }
}

// Every place's state as it is now: where its floating window is, and whether it is shown
static void note_places(Fl_Dockable_Area *area) {
  for (int i = 0; i < area->places(); i++) {
    area->place(i)->shown = false;
  }
  for (int i = 0; i < area->panels(); i++) {
    Fl_Dockable_Panel *d = area->panel(i);
    if (d->panel_shown()) {
      d->place()->shown = true;
    }
  }
}

/**
  Keeps the places as they are now as the default layout, which
  reset_layout() brings back.  An application calls it after making its
  places and panels, before reading a saved layout.
*/
void Fl_Dockable_Area::save_defaults() {
  defaults_.clear();
  for (size_t i = 0; i < places_.size(); i++) {
    defaults_.push_back(*places_[i]);
  }
}

/**
  Brings back the default layout: the panels shown are hidden through their
  hide_request(), the places take their defaults, floating windows and the
  sizes floating panels were given go, and the panels are shown again through
  their open_request() where the defaults put them, those that dock first so
  they have room.
*/
void Fl_Dockable_Area::reset_layout() {
  hide_docked(0);
  maximize(0);
  std::vector<Fl_Dockable_Panel *> shown;
  for (int pass = 0; pass < 2; pass++) {
    for (size_t i = 0; i < all_.size(); i++) {
      Fl_Dockable_Panel *d = all_[i];
      bool docks = true;
      for (size_t k = 0; k < places_.size() && k < defaults_.size(); k++) {
        if (places_[k] == d->place()) {
          docks = defaults_[k].docked;
        }
      }
      if (d->panel_shown() && docks == (pass == 0)) {
        shown.push_back(d);
      }
    }
  }
  for (size_t i = 0; i < shown.size(); i++) {
    shown[i]->hide_request();
  }
  for (size_t i = 0; i < places_.size() && i < defaults_.size(); i++) {
    Fl_Dockable_Panel *last = places_[i]->last;
    *places_[i] = defaults_[i];
    places_[i]->last = last;
  }
  for (size_t i = 0; i < all_.size(); i++) {
    all_[i]->float_size(0, 0);
    all_[i]->unfloat();
  }
  relayout();
  for (size_t i = 0; i < shown.size(); i++) {
    shown[i]->open_request();
  }
}

/**
  Writes every place, and the sizes floating panels were given, to \p prefs:
  each place in a group of "places", each panel's size in a group of
  "sizes".  Returns 1.
*/
int Fl_Dockable_Area::save_layout(Fl_Preferences &prefs) {
  for (size_t i = 0; i < all_.size(); i++) {
    all_[i]->keep_float_place();
  }
  note_places(this);
  // Only the sizes floating panels were given; one taken back by reset_layout() goes
  prefs.delete_group("sizes");
  Fl_Preferences sizes(prefs, "sizes");
  for (size_t i = 0; i < all_.size(); i++) {
    Fl_Dockable_Panel *d = all_[i];
    if (d->float_w() <= 0 || d->float_h() <= 0) {
      continue;
    }
    Fl_Preferences g(sizes, group_name(d->name()).c_str());
    g.set("w", d->float_w());
    g.set("h", d->float_h());
  }
  Fl_Preferences places(prefs, "places");
  for (size_t i = 0; i < places_.size(); i++) {
    Fl_Dockable_Place *p = places_[i];
    Fl_Preferences g(places, group_name(p->name).c_str());
    g.set("docked", (int)p->docked);
    g.set("side", p->side);
    g.set("order", p->order, 17);
    g.set("width", p->width);
    g.set("height", p->height);
    g.set("sub", p->sub, 17);
    g.set("extent", p->extent);
    g.set("cross", p->cross, 17);
    g.set("crossextent", p->cross_extent);
    g.set("stripe", p->stripe);
    g.set("stripeorder", p->stripe_order, 17);
    g.set("autohide", (int)p->auto_hide);
    g.set("shown", (int)p->shown);
    g.set("float", (int)p->float_set);
    g.set("captive", (int)p->float_captive);
    g.set("x", p->float_x);
    g.set("y", p->float_y);
  }
  prefs.flush();
  return 1;
}

/**
  Reads the places, and the sizes floating panels were given, that
  save_layout() wrote to \p prefs.  A place \p prefs does not name keeps what
  it has, and a value out of range keeps the place's own.  Panels are not
  shown or hidden; show_saved() shows those shown when it was saved.  Call it
  before the panels are first shown.  Returns 1 if \p prefs held a layout.
*/
int Fl_Dockable_Area::load_layout(Fl_Preferences &prefs) {
  int found = 0;
  if (prefs.group_exists("sizes")) {
    Fl_Preferences sizes(prefs, "sizes");
    for (size_t i = 0; i < all_.size(); i++) {
      std::string n = group_name(all_[i]->name());
      if (!sizes.group_exists(n.c_str())) {
        continue;
      }
      Fl_Preferences g(sizes, n.c_str());
      int W, H;
      g.get("w", W, 0);
      g.get("h", H, 0);
      if (W < 0 || H < 0) {
        W = 0;
        H = 0;
      }
      all_[i]->float_size(W, H);
    }
  }
  if (!prefs.group_exists("places")) {
    return found;
  }
  Fl_Preferences places(prefs, "places");
  for (size_t i = 0; i < places_.size(); i++) {
    Fl_Dockable_Place *p = places_[i];
    std::string n = group_name(p->name);
    if (!places.group_exists(n.c_str())) {
      continue;
    }
    found = 1;
    Fl_Preferences g(places, n.c_str());
    Fl_Dockable_Place def = *p;
    int docked, autohide, shown, floatset, captive;
    g.get("docked", docked, (int)p->docked);
    g.get("side", p->side, p->side);
    g.get("order", p->order, p->order);
    g.get("width", p->width, p->width);
    g.get("height", p->height, p->height);
    g.get("sub", p->sub, p->sub);
    g.get("extent", p->extent, p->extent);
    g.get("cross", p->cross, p->cross);
    g.get("crossextent", p->cross_extent, p->cross_extent);
    g.get("stripe", p->stripe, p->stripe);
    g.get("stripeorder", p->stripe_order, p->stripe_order);
    g.get("autohide", autohide, (int)p->auto_hide);
    g.get("shown", shown, (int)p->shown);
    g.get("float", floatset, (int)p->float_set);
    g.get("captive", captive, (int)p->float_captive);
    g.get("x", p->float_x, p->float_x);
    g.get("y", p->float_y, p->float_y);
    p->docked = (docked != 0);
    p->auto_hide = (autohide != 0);
    p->shown = (shown != 0);
    p->float_set = (floatset != 0);
    p->float_captive = (captive != 0);
    check_place(p, def);
  }
  relayout();
  return found;
}

/** Writes the layout to the group \p name of \p prefs, a layout of that name. */
int Fl_Dockable_Area::save_layout(Fl_Preferences &prefs, const char *name) {
  Fl_Preferences layouts(prefs, "layouts");
  std::string n;
  if (name) {
    n = name;
  }
  Fl_Preferences g(layouts, group_name(n).c_str());
  return save_layout(g);
}

/** Reads the layout of that name from \p prefs; returns 0 if there is none. */
int Fl_Dockable_Area::load_layout(Fl_Preferences &prefs, const char *name) {
  if (!prefs.group_exists("layouts")) {
    return 0;
  }
  Fl_Preferences layouts(prefs, "layouts");
  std::string n;
  if (name) {
    n = name;
  }
  n = group_name(n);
  if (!layouts.group_exists(n.c_str())) {
    return 0;
  }
  Fl_Preferences g(layouts, n.c_str());
  return load_layout(g);
}

/**
  Shows the panels of the places shown when the layout was saved, through
  open_request(), docked ones first.
*/
void Fl_Dockable_Area::show_saved() {
  for (int pass = 0; pass < 2; pass++) {
    for (size_t i = 0; i < places_.size(); i++) {
      Fl_Dockable_Place *p = places_[i];
      if (p->shown && p->docked == (pass == 0)) {
        show_place(p);
      }
    }
  }
}

// A name in a layout's text: a run of characters, with space, '%', '=' and line ends as %XX
static void put_name(std::string &out, const std::string &name) {
  for (size_t i = 0; i < name.size(); i++) {
    unsigned char c = (unsigned char)name[i];
    if (c <= ' ' || c == '%' || c == '=' || c >= 127) {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", c);
      out += buf;
    } else {
      out += (char)c;
    }
  }
}

static std::string get_name(const char *&s) {
  std::string n;
  while (*s && *s != ' ' && *s != '\n') {
    if (*s == '%' && s[1] && s[2]) {
      char hex[3] = {s[1], s[2], 0};
      n += (char)strtol(hex, 0, 16);
      s += 3;
    } else {
      n += *s++;
    }
  }
  return n;
}

/**
  Writes the layout as text to \p buf, at most \p size bytes with the
  terminating nul, as snprintf() does: one line for each place,
  "place NAME key=value ...", and one for each panel's floating size,
  "size NAME W H".  Returns the length of the whole text, which is
  larger than \p size - 1 if it did not fit.
*/
int Fl_Dockable_Area::write_layout(char *buf, int size) {
  for (size_t i = 0; i < all_.size(); i++) {
    all_[i]->keep_float_place();
  }
  note_places(this);
  std::string out;
  char line[1024];
  for (size_t i = 0; i < places_.size(); i++) {
    Fl_Dockable_Place *p = places_[i];
    out += "place ";
    put_name(out, p->name);
    snprintf(line, sizeof(line),
             " docked=%d side=%d order=%.17g width=%d height=%d sub=%.17g extent=%d cross=%.17g"
             " crossextent=%d stripe=%d"
             " stripeorder=%.17g autohide=%d shown=%d float=%d captive=%d x=%d y=%d\n",
             (int)p->docked, p->side, p->order, p->width, p->height, p->sub, p->extent, p->cross,
             p->cross_extent, p->stripe,
             p->stripe_order, (int)p->auto_hide, (int)p->shown, (int)p->float_set,
             (int)p->float_captive, p->float_x, p->float_y);
    out += line;
  }
  for (size_t i = 0; i < all_.size(); i++) {
    if (all_[i]->float_w() <= 0) {
      continue;
    }
    out += "size ";
    put_name(out, all_[i]->name());
    snprintf(line, sizeof(line), " %d %d\n", all_[i]->float_w(), all_[i]->float_h());
    out += line;
  }
  if (buf && size > 0) {
    snprintf(buf, size, "%s", out.c_str());
  }
  return (int)out.size();
}

/**
  Reads a layout write_layout() wrote.  Places and panels the text does not
  name keep what they have; unknown keys and lines are skipped, and values out
  of range keep the place's own.  Returns 1 if the text named a place.
*/
int Fl_Dockable_Area::read_layout(const char *text) {
  int found = 0;
  const char *s = text;
  while (s && *s) {
    while (*s == ' ' || *s == '\n' || *s == '\r' || *s == '\t') {
      s++;
    }
    if (!*s) {
      break;
    }
    if (!strncmp(s, "place ", 6)) {
      s += 6;
      Fl_Dockable_Place *p = find_place(get_name(s).c_str());
      Fl_Dockable_Place def;
      if (p) {
        def = *p;
        found = 1;
      }
      while (*s == ' ') {
        s++;
        const char *eq = strchr(s, '=');
        const char *end = s + strcspn(s, " \n");
        if (!eq || eq > end) {
          s = end;
          continue;
        }
        std::string key(s, eq - s);
        double v = strtod(eq + 1, 0);
        s = end;
        if (!p) {
          continue;
        }
        if (key == "docked") p->docked = (v != 0);
        else if (key == "side") p->side = (int)v;
        else if (key == "order") p->order = v;
        else if (key == "width") p->width = (int)v;
        else if (key == "height") p->height = (int)v;
        else if (key == "sub") p->sub = v;
        else if (key == "extent") p->extent = (int)v;
        else if (key == "cross") p->cross = v;
        else if (key == "crossextent") p->cross_extent = (int)v;
        else if (key == "stripe") p->stripe = (int)v;
        else if (key == "stripeorder") p->stripe_order = v;
        else if (key == "autohide") p->auto_hide = (v != 0);
        else if (key == "shown") p->shown = (v != 0);
        else if (key == "float") p->float_set = (v != 0);
        else if (key == "captive") p->float_captive = (v != 0);
        else if (key == "x") p->float_x = (int)v;
        else if (key == "y") p->float_y = (int)v;
      }
      if (p) {
        check_place(p, def);
      }
    } else if (!strncmp(s, "size ", 5)) {
      s += 5;
      Fl_Dockable_Panel *d = find(get_name(s).c_str());
      int W = (int)strtol(s, (char **)&s, 10);
      int H = (int)strtol(s, (char **)&s, 10);
      if (d && W >= 0 && H >= 0) {
        d->float_size(W, H);
      }
    }
    s += strcspn(s, "\n");
  }
  relayout();
  return found;
}

/**
  Lists every panel in \p menu under \p path, such as "Window/Panels", each
  item checked while its panel is shown; choosing one shows or hides it
  through its open_request() or hide_request().  The list follows the panels
  as they are made, shown and hidden.  Pass 0 to stop.
*/
void Fl_Dockable_Area::panel_menu(Fl_Menu_ *menu, const char *path) {
  if (menu_tracker_ && !menu_tracker_->deleted() && !panel_menu_path_.empty()) {
    int i = panel_menu_->find_index(panel_menu_path_.c_str());
    if (i >= 0) {
      panel_menu_->clear_submenu(i);
    }
  }
  delete menu_tracker_;
  menu_tracker_ = 0;
  panel_menu_ = menu;
  panel_menu_path_ = "";
  if (menu && path) {
    menu_tracker_ = new Fl_Widget_Tracker(menu);
    panel_menu_path_ = path;
  }
  rebuild_panel_menu();
}

// A title as a menu label: '/', '&', '_' and '\' escaped
static std::string menu_label(const char *t) {
  std::string out;
  for (const char *c = t; *c; c++) {
    if (*c == '/' || *c == '&' || *c == '_' || *c == '\\') {
      out += '\\';
    }
    out += *c;
  }
  return out;
}

// The list follows the panels at the next turn of the event loop, not from inside a menu's callback
void Fl_Dockable_Area::update_panel_menu() {
  if (!menu_tracker_ || menu_pending_) {
    return;
  }
  menu_pending_ = true;
  Fl::add_timeout(0, menu_timeout_cb, this);
}

void Fl_Dockable_Area::menu_timeout_cb(void *data) {
  Fl_Dockable_Area *area = (Fl_Dockable_Area *)data;
  area->menu_pending_ = false;
  area->rebuild_panel_menu();
}

void Fl_Dockable_Area::rebuild_panel_menu() {
  if (!menu_tracker_ || menu_tracker_->deleted() || panel_menu_path_.empty()) {
    return;
  }
  int i = panel_menu_->find_index(panel_menu_path_.c_str());
  if (i >= 0) {
    panel_menu_->clear_submenu(i);
  }
  std::vector<Fl_Dockable_Panel *> sorted = all_;
  std::stable_sort(sorted.begin(), sorted.end(), [](Fl_Dockable_Panel *a, Fl_Dockable_Panel *b) {
    return strcmp(a->title(), b->title()) < 0;
  });
  for (size_t k = 0; k < sorted.size(); k++) {
    Fl_Dockable_Panel *d = sorted[k];
    std::string item = panel_menu_path_ + "/" + menu_label(d->title());
    int flags = FL_MENU_TOGGLE;
    if (d->panel_shown()) {
      flags |= FL_MENU_VALUE;
    }
    panel_menu_->add(item.c_str(), d->dock_shortcut(), panel_menu_cb, d, flags);
  }
}

void Fl_Dockable_Area::panel_menu_cb(Fl_Widget *w, void *data) {
  Fl_Dockable_Panel *d = (Fl_Dockable_Panel *)data;
  Fl_Dockable_Area *area = d->area();
  if (!area) {
    return;
  }
  area->request(d, !d->panel_shown());
}

/**
  A key no widget took: Escape slides a panel slid out away, and a panel's
  dock_shortcut() shows or hides it.  Returns 1 if the key was used.
*/
int Fl_Dockable_Area::key_pressed() {
  if (Fl::event_key() == FL_Escape && slid_) {
    request(slid_, 0);
    if (view_) {
      view_->take_focus();
    }
    return 1;
  }
  for (size_t i = 0; i < all_.size(); i++) {
    Fl_Dockable_Panel *d = all_[i];
    if (!d->dock_shortcut() || !Fl::test_shortcut(d->dock_shortcut())) {
      continue;
    }
    request(d, !d->panel_shown());
    return 1;
  }
  return 0;
}

// What the user asked of a panel: the application shows or hides it, and is told, even if its
// request deleted the panel
void Fl_Dockable_Area::request(Fl_Dockable_Panel *d, int open) {
  Fl_Widget_Tracker alive(d);
  if (open) {
    d->open_request();
  } else {
    d->hide_request();
  }
  relayout();
  Fl_Dockable_Panel *about = d;
  if (alive.deleted()) {
    about = 0;
  }
  if (open) {
    notify(about, FL_REASON_OPENED);
  } else {
    notify(about, FL_REASON_CLOSED);
  }
}
