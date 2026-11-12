//
// Dockable panel header file for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

#ifndef Fl_Dockable_Panel_H
#define Fl_Dockable_Panel_H

#include <FL/Fl_Dockable_Area.H>

#include <string>
#include <vector>

class Fl_Dockable_Bar;
class Fl_Image;

/**
  A panel of an Fl_Dockable_Area: a title bar above a body group holding the
  application's widgets.  The user drags it by its title bar out of the area,
  to float it, and back to dock it.  Floating, the title bar is its window's.

  Widgets created between begin_body() and end_body() go into the body.  Or
  adopt() moves the widgets of a group, such as a window the application
  built, into the body.
  \code
  Fl_Dockable_Panel *p = new Fl_Dockable_Panel(area, 300, 400, "Files");
  p->begin_body();
  Fl_Browser *b = new Fl_Browser(0, 0, 300, 400);
  p->end_body();
  p->body()->resizable(b);
  p->show_panel();
  \endcode
  The body's widgets are placed relative to the body's corner when it is at
  0, 0, as in a window.

  The application learns what the user asks of a panel through its virtual
  functions, which an application overrides to run its own code: a close
  button pressed calls close_request(), a stripe button calls open_request()
  or hide_request().  The defaults show or hide the panel.

  A panel names its place, which says where it shows; panels that take turns
  in one place share it.  A panel made without one gets a place of its own,
  named by its title.  The area deletes the panels docked in it with itself;
  a panel floating or hidden is the application's to delete, and after the
  area is gone it does nothing but hide.  Show it with show_panel() and hide it with
  hide_panel().
*/
class FL_DOCKABLE_EXPORT Fl_Dockable_Panel : public Fl_Dockable_Group {
  friend class Fl_Dockable_Area;
  friend class Fl_Dockable_Bar;
  friend class Fl_Dockable_Tabs;
  friend class Fl_Dockable_Stripe;

public:
  Fl_Dockable_Panel(Fl_Dockable_Area *area, int W, int H, const char *title = 0,
                    Fl_Dockable_Place *place = 0);
  ~Fl_Dockable_Panel() override;

  int handle(int event) override;
  void draw() override;
  void resize(int X, int Y, int W, int H) override;

  /** The group holding the application's widgets. */
  Fl_Group *body() const { return body_; }
  void begin_body();
  void end_body();
  void adopt(Fl_Group *from);
  void lend_body(Fl_Group *to);
  void take_back_body(Fl_Group *from);

  /** The area the panel docks in. */
  Fl_Dockable_Area *area() const { return area_; }
  /** The panel's place. */
  Fl_Dockable_Place *place() const { return place_; }
  void place(Fl_Dockable_Place *p);

  /** The title its title bar, tab, stripe button and window show. */
  const char *title() const { return title_.c_str(); }
  void title(const char *t);
  /** The name the panel's sizes are saved under; its title unless set. */
  const char *name() const;
  /** Sets the name the panel's sizes are saved under. */
  void name(const char *n) {
    if (n)
      name_ = n;
    else
      name_ = "";
  }

  void show_panel();
  void hide_panel();
  int panel_shown() const;
  int docked() const;

  void float_panel(int X, int Y, int captive);
  void float_as(int captive);
  void dock_panel();
  void dock_on(int side);
  void auto_hide();
  void pin();
  void show_button(int on);

  /**
    What the user may do with the panel, Fl_Dockable_Flags or'ed together.  A
    panel is dragged only if it may both move and float; the title bar's menu
    offers what the flags allow.
  */
  int dock_flags() const { return flags_; }
  /** Sets what the user may do with the panel. */
  void dock_flags(int f) { flags_ = f; }
  /** The icon on the panel's stripe button, before its title, or 0. */
  Fl_Image *dock_icon() const { return icon_; }
  /** Sets the icon on the panel's stripe button, about 16 pixels square; the application keeps it. */
  void dock_icon(Fl_Image *i) {
    icon_ = i;
    if (area_) {
      area_->relayout();
    }
  }
  /** The key that shows and hides the panel from any window of the application, or 0. */
  int dock_shortcut() const { return shortcut_; }
  void dock_shortcut(int s);

  /** The size the panel was built at, its title bar included. */
  int built_w() const { return built_w_; }
  /** \see built_w() */
  int built_h() const { return built_h_; }
  /** The size of the window it last floated in, or 0. */
  int float_w() const { return float_w_; }
  /** \see float_w() */
  int float_h() const { return float_h_; }
  /** Sets the size it floats at, or 0 for the size it was built at. */
  void float_size(int W, int H) {
    float_w_ = W;
    float_h_ = H;
  }

  /** The height of a panel's title bar. */
  static int bar_height() { return 22; }

  /** Called when the user closes the panel; the default hides it. */
  virtual void close_request() { hide_panel(); }
  /** Called when the user shows the panel from a stripe or a menu, or a layout is reset; the default shows it. */
  virtual void open_request() { show_panel(); }
  /** Called when the user hides the panel from its stripe, or the area hides it; the default hides it. */
  virtual void hide_request() { hide_panel(); }
  /** Called after the body is resized. */
  virtual void body_resized() {}

private:
  // A subwindow cannot move to another window while it is shown, so the panel's are hidden while it
  // moves, and shown again after
  void hide_subwindows();
  void show_subwindows();
  static void find_subwindows(Fl_Group *g, std::vector<Fl_Window *> &wins);
  void show_in_place();
  void hide_from_place();
  void settle();
  void user_close();
  void show_bar(bool on);
  void show_menu();
  void slide_out(int X, int Y, int W, int H);
  void unfloat();
  void stop_auto_hide();
  void keep_inside();
  void keep_float_place();
  void released();
  void in_window();
  void built_size();
  void rearm();
  static void window_cb(Fl_Widget *w, void *data);

  Fl_Dockable_Area *area_;
  Fl_Dockable_Place *place_;
  // A copy of its place it keeps after the area is deleted
  bool own_place_;
  std::string title_;
  std::string name_;
  int flags_;
  int shortcut_;
  Fl_Image *icon_;
  int built_w_;
  int built_h_;
  std::vector<Fl_Window *> subwindows_;
  Fl_Dockable_Bar *bar_;
  Fl_Group *body_;
  bool bar_shown_;
  bool settled_;
  Fl_Group *saved_current_;
  int float_w_;
  int float_h_;
  int slide_x_;
  int slide_y_;
  int lent_x_;
  int lent_y_;
  int lent_w_;
  int lent_h_;
};

#endif // !Fl_Dockable_Panel_H
