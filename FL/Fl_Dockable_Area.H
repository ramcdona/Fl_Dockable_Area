//
// Docking area header file for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#ifndef Fl_Dockable_Area_H
#define Fl_Dockable_Area_H

#include <FL/Fl_Dockable_Export.H>
#include <FL/Fl_Dockable_Version.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Dockable_Group.H>
#include <FL/Fl.H>

#include <string>
#include <vector>

class Fl_Box;
class Fl_Menu_;
class Fl_Preferences;
class Fl_Dockable_Panel;
class Fl_Dockable_Tabs;
class Fl_Dockable_Stack;
class Fl_Dockable_Stripe;

/** The borders of an Fl_Dockable_Area's center that panels dock along. */
enum Fl_Dockable_Side {
  FL_DOCKABLE_LEFT = 0,   ///< columns at the left
  FL_DOCKABLE_RIGHT,      ///< columns at the right
  FL_DOCKABLE_TOP,        ///< rows at the top
  FL_DOCKABLE_BOTTOM,     ///< rows at the bottom
  FL_DOCKABLE_NUM_SIDES   ///< the number of sides
};

/**
  What the user may do with a panel, or'ed together.
  \see Fl_Dockable_Panel::dock_flags()
*/
enum Fl_Dockable_Flags {
  FL_DOCKABLE_CLOSABLE  = 1,    ///< close it from its title bar, its window or its menu
  FL_DOCKABLE_FLOATABLE = 2,    ///< float it in a window
  FL_DOCKABLE_MOVABLE   = 4,    ///< drag it to another place
  FL_DOCKABLE_DEFAULT_FLAGS = 7 ///< all of the above
};

/**
  How the stripes behave, or'ed together.
  \see Fl_Dockable_Area::stripe_options()
*/
enum Fl_Dockable_Stripe_Options {
  FL_DOCKABLE_STRIPE_FOCUS_FIRST = 1, ///< a click on the button of a panel shown without the keyboard focus gives it the focus, rather than hiding it
  FL_DOCKABLE_STRIPE_SHOW_EMPTY = 2   ///< a stripe with no buttons is shown; otherwise it takes no room
};

/**
  Where a panel shows: docked along a border of the center, floating in a
  window, or sliding out from a button on a stripe.

  Panels that take turns in one place share one Fl_Dockable_Place: showing one
  of them hides the one shown there.  Every field may be read; the area changes
  them as the user moves panels, and an application may set them before a panel
  is first shown.  Places are made by Fl_Dockable_Area::add_place() and are
  kept by the area.
*/
struct FL_DOCKABLE_EXPORT Fl_Dockable_Place {
  Fl_Dockable_Place();

  /** The place's name, the key it is saved under; unique in its area. */
  std::string name;
  /** The border it docks along, an Fl_Dockable_Side. */
  int side;
  /** Where it goes among the places along its side: smaller is further left, or higher. */
  double order;
  /** Its width docked at the left or right, 0 for the panel's own width. */
  int width;
  /** Its height docked at the top or bottom, 0 for the panel's own height. */
  int height;
  /**
    Where it goes among the places stacked in one place, smaller first; places
    with the same side, order and sub are tabs.
  */
  double sub;
  /** Its size along a stack, 0 for a share. */
  int extent;
  /**
    Where it goes across a part of a stack, smaller first: places with the
    same side, order and sub but different cross stand side by side in a
    column, one above another in a row.
  */
  double cross;
  /** Its size across a part of a stack, 0 for a share. */
  int cross_extent;
  /** Whether it docks when shown; otherwise it floats. */
  bool docked;
  /** The stripe its button is on, an Fl_Dockable_Side, or -1 for none. */
  int stripe;
  /** Where its button goes along its stripe, smaller first. */
  double stripe_order;
  /** Whether it auto-hides: it slides out from its button when shown. */
  bool auto_hide;
  /** Whether it was shown as the layout was last saved. */
  bool shown;
  /** The panel in the place last shown, or 0. */
  Fl_Dockable_Panel *last;
  /** Whether its floating window's position is known. */
  bool float_set;
  /** Whether it floats captive, inside the area's window; otherwise in a window of its own. */
  bool float_captive;
  /** Its floating window's position: in its window's coordinates if captive, else the screen's. */
  int float_x;
  /** \see float_x */
  int float_y;
};

/**
  A center widget, such as a 3D view, with panels docked along its four
  borders: columns at the left and right the height of the area, rows at the
  top and bottom the width of the center.  The center takes whatever room the
  panels leave, down to its minimum size.

  The panels are Fl_Dockable_Panel groups.  The user drags a panel by its title
  bar: over the docked panels, or over a band of the center along a border, a
  placeholder shows where it will dock.  Over the middle of a docked panel it
  joins it as a tab; over its ends, it stacks before or after it in that place.
  Dropped elsewhere, or with Ctrl held, it floats: captive, in a window inside
  the area's window, over the area's captive layer, or in a window of its own
  outside it.  The borders between the center and the docked panels can be
  dragged.

  Each panel's title bar has a menu: dock it along a border, float it inside or
  in a window of its own, auto-hide it on a stripe, give it a button on a
  stripe, maximize it, or close it.  A stripe along a border holds a button for
  each panel given one; a click shows or hides the panel, and an auto-hidden
  panel slides out from it over the center, and away again when the user clicks
  elsewhere or presses Escape.

  The area tells the application what the user did through its callback, with
  Fl::callback_reason() set and callback_panel() returning the panel:
  - FL_REASON_DRAGGED: the user moved a panel, docked it, floated it, gave it a
    button or took it away, auto-hid or pinned it.
  - FL_REASON_CLOSED: the user hid a panel.
  - FL_REASON_OPENED: the user showed a panel.
  - FL_REASON_CHANGED: the user resized something, or maximized a panel or put
    it back; callback_panel() may be 0.
  An application that saves its layout as it changes can save on every call.
  A panel's functions that move it, such as float_panel() or dock_on(), call
  it too, with FL_REASON_DRAGGED.

  The area sees every event through an Fl::event_dispatch() it puts in front
  of the one set before it, and takes it away again when it is deleted: a
  click away from a panel slid out, or Escape, slides it away.  A panel's
  dock_shortcut() works from any window of the application through an
  Fl::add_handler() function.
*/
class FL_DOCKABLE_EXPORT Fl_Dockable_Area : public Fl_Tile {
  friend class Fl_Dockable_Panel;
  friend class Fl_Dockable_Tabs;
  friend class Fl_Dockable_Stack;
  friend class Fl_Dockable_Stripe;

public:
  /** Creates a panel a layout or show_place() names before the application has made it. */
  typedef Fl_Dockable_Panel *(*Panel_Factory)(Fl_Dockable_Area *area, Fl_Dockable_Place *place,
                                              void *data);

  Fl_Dockable_Area(int X, int Y, int W, int H, Fl_Widget *center);
  ~Fl_Dockable_Area() override;

  int handle(int event) override;
  void resize(int X, int Y, int W, int H) override;

  /** The center widget. */
  Fl_Widget *center() const { return view_; }

  /**
    Sets the group panels float captive in: a group over the area, in the same
    window, holding only floating windows, usually an Fl_Dockable_Layer.  With
    none, panels float only in windows of their own.
  */
  void captive_layer(Fl_Group *g);
  /** The group panels float captive in, or 0. */
  Fl_Group *captive_layer() const { return captive_; }

  // Places
  Fl_Dockable_Place *add_place(const char *name, int side = FL_DOCKABLE_RIGHT, double order = 0);
  Fl_Dockable_Place *find_place(const char *name) const;
  /** The number of places. */
  int places() const { return (int)places_.size(); }
  /** Place \p i, or 0. */
  Fl_Dockable_Place *place(int i) const;
  int show_place(Fl_Dockable_Place *p);
  /** Sets the function that makes the panel of a place that has none. */
  void panel_factory(Panel_Factory f, void *data = 0) {
    factory_ = f;
    factory_data_ = data;
  }

  // Panels
  /** The number of panels. */
  int panels() const { return (int)all_.size(); }
  /** Panel \p i, or 0. */
  Fl_Dockable_Panel *panel(int i) const;
  Fl_Dockable_Panel *find(const char *name) const;

  void maximize(Fl_Dockable_Panel *p);
  int maximized(Fl_Dockable_Panel *p);
  void hide_docked(int on);
  /** True while hide_docked() has the docked panels hidden. */
  int docked_hidden() const { return !hidden_docked_.empty(); }

  // Layouts
  void save_defaults();
  void reset_layout();
  int save_layout(Fl_Preferences &prefs);
  int load_layout(Fl_Preferences &prefs);
  int save_layout(Fl_Preferences &prefs, const char *name);
  int load_layout(Fl_Preferences &prefs, const char *name);
  int write_layout(char *buf, int size);
  int read_layout(const char *text);
  void show_saved();

  void panel_menu(Fl_Menu_ *menu, const char *path);

  /** The panel the last callback was about, or 0. */
  Fl_Dockable_Panel *callback_panel() const { return callback_panel_; }

  /** Sets the size the center keeps as panels dock beside it. */
  void min_center(int W, int H) {
    min_view_w_ = W;
    min_view_h_ = H;
  }
  /** The width the center keeps. */
  int min_center_w() const { return min_view_w_; }
  /** The height the center keeps. */
  int min_center_h() const { return min_view_h_; }
  /** Sets the width of the band of the center along a border over which a dragged panel docks. */
  void dock_band(int b) { dock_band_ = b; }
  /** The width of the band of the center a dragged panel docks over. */
  int dock_band() const { return dock_band_; }
  /** Sets how the stripes behave, Fl_Dockable_Stripe_Options or'ed together. */
  void stripe_options(int o) {
    stripe_options_ = o;
    relayout();
  }
  /** How the stripes behave. */
  int stripe_options() const { return stripe_options_; }
  /** The thickness of a stripe with buttons. */
  static int stripe_thickness() { return 24; }

  int fits(Fl_Dockable_Panel *d, int side);
  void relayout();

  static int dispatch(int event, Fl_Window *w);
  int key_pressed();

  static int api_version();

private:
  static bool inside_cb(Fl_Widget *target);
  static bool acceptable_cb(Fl_Widget *target, Fl_Dockable_Group *dockable);
  static void state_cb(Fl_Widget *target, bool can_dock);
  static void payload_cb(Fl_Widget *target, Fl_Dockable_Group *dockable);
  static void undock_cb(Fl_Widget *target, Fl_Dockable_Group *dockable);
  void inner_rect(int &X, int &Y, int &W, int &H);
  void register_panel(Fl_Dockable_Panel *d);
  void forget_panel(Fl_Dockable_Panel *d);
  void notify(Fl_Dockable_Panel *d, Fl_Callback_Reason reason);
  void show_column(Fl_Dockable_Panel *d);
  void hide_column(Fl_Dockable_Panel *d);
  int has_column(Fl_Dockable_Panel *d);
  int room_for(Fl_Dockable_Panel *d, int side);
  double free_order(Fl_Dockable_Panel *d, int side, double order);
  double free_sub(Fl_Dockable_Panel *d, int side, double order, double sub);
  static double free_step(double v);
  void hit_thickness(std::vector<int> th[], int t[]);
  static void hide_subwindows_in(Fl_Widget *c);
  static void show_subwindows_in(Fl_Widget *c);
  void request(Fl_Dockable_Panel *d, int open);
  static void menu_timeout_cb(void *data);
  double free_cross(Fl_Dockable_Panel *d, int side, double order, double sub, double cross);
  double cross_at(Fl_Widget *part, bool after);
  void place_by(Fl_Dockable_Panel *d, Fl_Widget *part, int kind);
  void add_at(Fl_Widget *node, Fl_Dockable_Panel *d, int level);
  void keep_extents(Fl_Widget *c);
  double order_next_to_view(int side, Fl_Dockable_Panel *d);
  void untabbed(Fl_Dockable_Tabs *tabs);
  void unstacked(Fl_Dockable_Stack *stack);
  std::vector<Fl_Dockable_Panel *> stripe_panels(int side);
  void redraw_stripes();
  void show_all_stripes(bool on);
  int stripe_at(int X_root, int Y_root, int &X, int &Y);
  void move_button(Fl_Dockable_Panel *d, int side, double order);
  void drag_button_over(Fl_Dockable_Panel *d, int X_root, int Y_root);
  void drop_panel_at(Fl_Dockable_Panel *d, int X_root, int Y_root);
  void show_stripe_hint(int side);
  double stripe_end(int side);
  Fl_Dockable_Stripe *stripe(int side) { return stripes_[side]; }
  void slide_out(Fl_Dockable_Panel *d);
  void slid_away(Fl_Dockable_Panel *d);
  Fl_Dockable_Panel *slid() { return slid_; }
  void pushed_at(Fl_Window *w);
  void borders_dragged();
  void keep_widths();
  int thickness(Fl_Widget *c, int side);
  int side_thickness(int side);
  int slot_along(int side, int pos, int start, Fl_Widget *&over, bool &edge);
  bool hit_test(int X_root, int Y_root, int &side, int &slot, Fl_Widget *&part, int &kind);
  void show_place_holder(int side, int slot, int thick);
  void remove_place_holder();
  void show_tab_hint(Fl_Widget *part, int kind);
  void remove_tab_hint();
  Fl_Widget *same_order(Fl_Dockable_Panel *d, int side);
  Fl_Widget *slot_of(Fl_Widget *c, int side);
  void join_tabs(Fl_Widget *part, Fl_Dockable_Panel *d);
  void add_to_place(Fl_Widget *entry, Fl_Dockable_Panel *d);
  void replace_in(Fl_Widget *old, Fl_Widget *now);
  void remove_entry(Fl_Widget *c);
  void forget_hidden(Fl_Widget *c);
  double sub_at(Fl_Widget *part, bool after);
  double order_at(int side, int slot);
  int side_of(Fl_Widget *c);
  void update_panel_menu();
  void rebuild_panel_menu();
  static void panel_menu_cb(Fl_Widget *w, void *data);

  Fl_Widget *view_;
  int min_view_w_;
  int min_view_h_;
  int dock_band_;
  // The docked panels are shrunk to fit a window too small for them
  bool shrunk_;
  std::vector<Fl_Dockable_Panel *> all_;
  std::vector<Fl_Dockable_Place *> places_;
  std::vector<Fl_Dockable_Place> defaults_;
  std::vector<Fl_Dockable_Panel *> hidden_docked_;
  Panel_Factory factory_;
  void *factory_data_;
  Fl_Dockable_Panel *callback_panel_;
  Fl_Dockable_Stripe *stripes_[FL_DOCKABLE_NUM_SIDES];
  bool all_stripes_;
  int stripe_options_;
  Fl_Dockable_Panel *slid_;
  Fl_Widget *maximized_;
  std::vector<Fl_Widget *> max_hidden_;
  Fl_Group *captive_;
  // The places docked along each side, in order, the placeholder among them while the mouse is there
  std::vector<Fl_Widget *> columns_[FL_DOCKABLE_NUM_SIDES];
  Fl_Box *place_holder_;
  // Over a docked panel a dragged one would join as a tab, or stack before or after
  Fl_Box *tab_hint_;
  Fl_Widget *tab_with_;
  int hint_kind_;
  int place_side_;
  int place_slot_;
  int place_thick_;
  Fl_Menu_ *panel_menu_;
  Fl_Widget_Tracker *menu_tracker_;
  bool menu_pending_;
  // Panels floated for want of room step down and right from the center's corner
  int cascade_;
  std::string panel_menu_path_;
};

#endif // !Fl_Dockable_Area_H
