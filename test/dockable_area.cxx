//
// Demo program for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

// A center view with panels around it.  Drag a panel's title bar to dock it
// along a border, as a tab of another (over its middle), stacked before or
// after it (over its ends) or across a part of a stack (over its sides), or to
// float it, inside this window or, dragged out of it, in a window of its own;
// Ctrl floats it where it is dropped.  The title bar's menu, or the right mouse
// button on it, docks, floats, auto-hides, gives a stripe button, maximizes and
// closes a panel.
//
// The panels show what an application can do:
// - Files has a key, Alt+1, and an icon on its stripe button.
// - Properties cannot be closed, and stacks above Outline in one place.
// - Output and Log are tabs; Log lists what the area's callback reports.
// - Tools auto-hides: it slides out from its button on the left stripe and
//   away again on a click elsewhere or Escape.
// - Find starts floating inside this window.
// - Box Editor and Text Editor take turns in one place: showing one hides the
//   other.
// - Notes asks before it closes.
// - Settings was built as a window of its own and adopted.
// - View is an OpenGL window, which keeps drawing as its panel moves.
// - Help is not made until the Window menu first shows it.
// The Window menu lists the panels; the Layout menu saves and uses named
// layouts and prints the layout as text; the Options menu changes how the
// stripes behave.  The layout is kept in the user's preferences.

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Preferences.H>
#include <FL/fl_ask.H>
#include <FL/gl.h>
#include <FL/Fl_Dockable_Area.H>
#include <FL/Fl_Dockable_Layer.H>
#include <FL/Fl_Dockable_Panel.H>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Fl_Dockable_Area *area = 0;
static Fl_Browser *log_browser = 0;

static const char *PREFS_VENDOR = "fltk.org";
static const char *PREFS_APP = "dockable_area_demo";

// ---- The panels --------------------------------------------------------------

// A panel of a browser saying what it is for
static Fl_Dockable_Panel *list_panel(const char *title, Fl_Dockable_Place *place, int W, int H,
                                     const char *about) {
  Fl_Dockable_Panel *p = new Fl_Dockable_Panel(area, W, H, title, place);
  p->begin_body();
  Fl_Browser *b = new Fl_Browser(5, 5, W - 10, H - 10);
  b->add(title);
  b->add(about);
  p->end_body();
  p->body()->resizable(b);
  return p;
}

// A panel that asks before it closes
class Notes_Panel : public Fl_Dockable_Panel {
public:
  Notes_Panel(Fl_Dockable_Place *place) : Fl_Dockable_Panel(::area, 260, 160, "Notes", place) {
    begin_body();
    Fl_Multiline_Input *in = new Fl_Multiline_Input(5, 5, 250, 150);
    in->value("Closing this panel asks first.");
    end_body();
    body()->resizable(in);
  }
  void close_request() override {
    if (fl_choice("Close the notes?", "Keep", "Close", 0) == 1) {
      hide_panel();
    }
  }
};

// A spinning square, which keeps drawing as its panel docks, floats, and is
// hidden and shown again
class Spin_View : public Fl_Gl_Window {
public:
  Spin_View(int X, int Y, int W, int H) : Fl_Gl_Window(X, Y, W, H), angle_(0) {
    Fl::add_timeout(0.05, tick, this);
  }
  ~Spin_View() override { Fl::remove_timeout(tick, this); }
  void draw() override {
    glViewport(0, 0, pixel_w(), pixel_h());
    glClearColor(0.15f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef((float)angle_, 0.0f, 0.0f, 1.0f);
    glColor3f(0.9f, 0.7f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f(0.5f, -0.5f);
    glVertex2f(0.5f, 0.5f);
    glVertex2f(-0.5f, 0.5f);
    glEnd();
  }

private:
  static void tick(void *data) {
    Spin_View *v = (Spin_View *)data;
    v->angle_ += 3;
    if (v->visible_r()) {
      v->redraw();
    }
    Fl::repeat_timeout(0.05, tick, data);
  }
  double angle_;
};

// The Help panel, made by the area's factory the first time it is shown
static Fl_Dockable_Panel *factory(Fl_Dockable_Area *, Fl_Dockable_Place *place, void *) {
  if (place->name != "Help") {
    return 0;
  }
  return list_panel("Help", place, 260, 200, "Made when the Window menu first showed it.");
}

// A 16 by 16 icon: a blue square with a lighter frame
static Fl_RGB_Image *make_icon() {
  static unsigned char pixels[16 * 16 * 3];
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      unsigned char *p = pixels + 3 * (16 * y + x);
      p[0] = 40;
      p[1] = 90;
      p[2] = 200;
      if (x < 2 || y < 2 || x > 13 || y > 13) {
        p[0] = 150;
        p[1] = 190;
        p[2] = 250;
      }
    }
  }
  return new Fl_RGB_Image(pixels, 16, 16, 3);
}

// ---- The area's callback --------------------------------------------------------

static void save_layout() {
  Fl_Preferences prefs(Fl_Preferences::USER_L, PREFS_VENDOR, PREFS_APP);
  area->save_layout(prefs);
}

// Logs what the user did, and keeps the layout
static void area_cb(Fl_Widget *, void *) {
  const char *what = "changed";
  Fl_Callback_Reason r = Fl::callback_reason();
  if (r == FL_REASON_DRAGGED) {
    what = "moved";
  } else if (r == FL_REASON_CLOSED) {
    what = "closed";
  } else if (r == FL_REASON_OPENED) {
    what = "opened";
  }
  char line[200];
  Fl_Dockable_Panel *p = area->callback_panel();
  if (p) {
    snprintf(line, sizeof(line), "%s: %s", what, p->title());
  } else {
    snprintf(line, sizeof(line), "%s", what);
  }
  if (log_browser) {
    log_browser->add(line);
    log_browser->bottomline(log_browser->size());
  }
  save_layout();
}

// ---- Menus -----------------------------------------------------------------------

static void quit_cb(Fl_Widget *, void *) {
  save_layout();
  exit(0);
}

static void show_place_cb(Fl_Widget *, void *data) {
  area->show_place(area->find_place((const char *)data));
}

static void show_panel_cb(Fl_Widget *, void *data) {
  Fl_Dockable_Panel *p = area->find((const char *)data);
  if (p) {
    p->show_panel();
  }
}

static void hide_docked_cb(Fl_Widget *, void *) {
  area->hide_docked(!area->docked_hidden());
}

static void maximize_cb(Fl_Widget *, void *) {
  Fl_Dockable_Panel *p = area->find("Properties");
  if (!p || area->maximized(p)) {
    area->maximize(0);
  } else if (p->docked()) {
    area->maximize(p);
  }
}

static void reset_cb(Fl_Widget *, void *) {
  area->reset_layout();
  save_layout();
}

// Saves (s) or uses (l) a named layout, as "sA" or "lB"
static void named_cb(Fl_Widget *, void *data) {
  const char *what = (const char *)data;
  Fl_Preferences prefs(Fl_Preferences::USER_L, PREFS_VENDOR, PREFS_APP);
  if (what[0] == 's') {
    area->save_layout(prefs, what + 1);
  } else if (!area->load_layout(prefs, what + 1)) {
    fl_alert("There is no layout %s yet.", what + 1);
  } else {
    area->show_saved();
  }
}

static void print_cb(Fl_Widget *, void *) {
  int n = area->write_layout(0, 0);
  char *text = (char *)malloc(n + 1);
  area->write_layout(text, n + 1);
  printf("%s", text);
  free(text);
}

static void option_cb(Fl_Widget *w, void *data) {
  int option = (int)(fl_intptr_t)data;
  int o = area->stripe_options();
  if (((Fl_Menu_ *)w)->mvalue()->value()) {
    o |= option;
  } else {
    o &= ~option;
  }
  area->stripe_options(o);
}

// ---- Main --------------------------------------------------------------------

int main(int argc, char **argv) {
  Fl_Double_Window *win = new Fl_Double_Window(1200, 800, "Fl_Dockable_Area demo");
  Fl_Menu_Bar *menu = new Fl_Menu_Bar(0, 0, 1200, 25);
  menu->add("&File/&Quit", FL_COMMAND + 'q', quit_cb);
  menu->add("&Window/Panels", 0, 0, 0, FL_SUBMENU);
  menu->add("&Window/Show &Box Editor", 0, show_panel_cb, (void *)"Box Editor");
  menu->add("&Window/Show &Text Editor", 0, show_panel_cb, (void *)"Text Editor");
  menu->add("&Window/Show &Help", 0, show_place_cb, (void *)"Help", FL_MENU_DIVIDER);
  menu->add("&Window/&Maximize Properties", 0, maximize_cb);
  menu->add("&Window/&Hide Docked Panels", 0, hide_docked_cb);
  menu->add("&Window/&Reset Layout", 0, reset_cb);
  menu->add("&Layout/Save as &A", 0, named_cb, (void *)"sA");
  menu->add("&Layout/Use A", 0, named_cb, (void *)"lA");
  menu->add("&Layout/Save as &B", 0, named_cb, (void *)"sB");
  menu->add("&Layout/Use B", 0, named_cb, (void *)"lB", FL_MENU_DIVIDER);
  menu->add("&Layout/&Print", 0, print_cb);
  menu->add("&Options/Click Gives &Focus First", 0, option_cb,
            (void *)(fl_intptr_t)FL_DOCKABLE_STRIPE_FOCUS_FIRST, FL_MENU_TOGGLE);
  menu->add("&Options/Show &Empty Stripes", 0, option_cb,
            (void *)(fl_intptr_t)FL_DOCKABLE_STRIPE_SHOW_EMPTY, FL_MENU_TOGGLE);
  win->end();

  // The area does not become the current group; the layer over it holds the
  // windows of panels floating inside this window
  Fl_Group::current(0);
  Fl_Box *center = new Fl_Box(FL_DOWN_BOX, 0, 0, 10, 10, "Center");
  center->color(FL_WHITE);
  area = new Fl_Dockable_Area(0, 25, 1200, 775, center);
  area->min_center(200, 150);
  win->add(area);
  Fl_Dockable_Layer *layer = new Fl_Dockable_Layer(0, 25, 1200, 775);
  win->add(layer);
  area->captive_layer(layer);
  win->resizable(area);

  // Where the panels go by default
  Fl_Dockable_Place *files = area->add_place("Files", FL_DOCKABLE_LEFT, 0);
  files->stripe = FL_DOCKABLE_LEFT;
  Fl_Dockable_Place *tools = area->add_place("Tools", FL_DOCKABLE_LEFT, 1);
  tools->stripe = FL_DOCKABLE_LEFT;
  tools->auto_hide = true;
  Fl_Dockable_Place *props = area->add_place("Properties", FL_DOCKABLE_RIGHT, 0);
  props->stripe = FL_DOCKABLE_RIGHT;
  Fl_Dockable_Place *outline = area->add_place("Outline", FL_DOCKABLE_RIGHT, 0);
  outline->sub = 1;
  Fl_Dockable_Place *output = area->add_place("Output", FL_DOCKABLE_BOTTOM, 0);
  Fl_Dockable_Place *log = area->add_place("Log", FL_DOCKABLE_BOTTOM, 0);
  Fl_Dockable_Place *editor = area->add_place("Editor", FL_DOCKABLE_RIGHT, 1);
  Fl_Dockable_Place *notes = area->add_place("Notes", FL_DOCKABLE_TOP, 0);
  Fl_Dockable_Place *find = area->add_place("Find", FL_DOCKABLE_RIGHT, 2);
  find->docked = false;
  find->float_set = true;
  find->float_captive = true;
  find->float_x = 400;
  find->float_y = 100;
  Fl_Dockable_Place *settings = area->add_place("Settings", FL_DOCKABLE_LEFT, 2);
  Fl_Dockable_Place *view = area->add_place("View", FL_DOCKABLE_BOTTOM, 1);
  area->add_place("Help", FL_DOCKABLE_RIGHT, 3);

  Fl_Dockable_Panel *p = list_panel("Files", files, 220, 400, "Alt+1 shows and hides it.");
  p->dock_shortcut(FL_ALT + '1');
  p->dock_icon(make_icon());
  list_panel("Tools", tools, 200, 300, "It slides out from the left stripe.");
  p = list_panel("Properties", props, 260, 300, "It cannot be closed.");
  p->dock_flags(FL_DOCKABLE_MOVABLE | FL_DOCKABLE_FLOATABLE);
  p->dock_shortcut(FL_ALT + '2');
  list_panel("Outline", outline, 260, 200, "Stacked below Properties.");
  list_panel("Output", output, 600, 150, "A tab beside Log.");
  Fl_Dockable_Panel *log_panel = new Fl_Dockable_Panel(area, 600, 150, "Log", log);
  log_panel->begin_body();
  log_browser = new Fl_Browser(5, 5, 590, 140);
  log_panel->end_body();
  log_panel->body()->resizable(log_browser);
  // Two panels that take turns in one place
  Fl_Dockable_Panel *box_editor = new Fl_Dockable_Panel(area, 260, 200, "Box Editor", editor);
  box_editor->begin_body();
  new Fl_Value_Slider(10, 30, 240, 25, "Width");
  new Fl_Value_Slider(10, 80, 240, 25, "Height");
  box_editor->end_body();
  Fl_Dockable_Panel *text_editor = new Fl_Dockable_Panel(area, 260, 200, "Text Editor", editor);
  text_editor->begin_body();
  new Fl_Input(60, 20, 190, 25, "Text:");
  new Fl_Check_Button(60, 60, 190, 25, "Bold");
  text_editor->end_body();
  new Notes_Panel(notes);
  Fl_Dockable_Panel *f = new Fl_Dockable_Panel(area, 300, 60, "Find", find);
  f->begin_body();
  new Fl_Input(60, 15, 220, 25, "Find:");
  f->end_body();
  // A window built on its own, adopted by a panel
  Fl_Group::current(0);
  Fl_Window *built = new Fl_Window(220, 120, "Settings");
  new Fl_Check_Button(10, 10, 200, 25, "Snap to grid");
  new Fl_Check_Button(10, 40, 200, 25, "Show axes");
  new Fl_Value_Slider(10, 80, 200, 25);
  built->end();
  Fl_Dockable_Panel *s = new Fl_Dockable_Panel(area, 10, 10, "Settings", settings);
  s->adopt(built);
  delete built;
  Fl_Dockable_Panel *v = new Fl_Dockable_Panel(area, 300, 200, "View", view);
  v->begin_body();
  v->body()->resizable(new Spin_View(5, 5, 290, 190));
  v->end_body();
  area->panel_factory(factory);

  area->panel_menu(menu, "&Window/Panels");
  area->save_defaults();
  Fl_Preferences prefs(Fl_Preferences::USER_L, PREFS_VENDOR, PREFS_APP);
  if (!area->load_layout(prefs)) {
    files->shown = true;
    props->shown = true;
    outline->shown = true;
    output->shown = true;
    log->shown = true;
    find->shown = true;
  }
  area->callback(area_cb);
  win->callback(quit_cb);
  win->show(argc, argv);
  area->show_saved();
  return Fl::run();
}
