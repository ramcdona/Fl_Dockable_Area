//
// Unit tests for the Fl_Dockable_Area extension to the Fast Light Tool Kit (FLTK).
//
// Copyright 2026 by Rob McDonald and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//

// These tests build areas in windows that are shown, as a panel is shown only
// in a window shown.  Floating and dragging are tested with a real mouse in the
// demo.

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/platform.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Dockable_Area.H>
#include <FL/Fl_Dockable_Layer.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Dockable_Panel.H>
#include "Fl_Dockable_Bar.H"
#include "Fl_Dockable_Tabs.H"
#include "Fl_Dockable_Stack.H"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

// ---- A small test harness, in the style of FLTK's unittests ---------------

typedef bool (*Test_Function)();

struct Test_Case {
  const char *suite;
  const char *name;
  Test_Function fn;
};

static const int MAX_TEST_CASES = 400;
static Test_Case test_cases[MAX_TEST_CASES];
static int num_test_cases = 0;

struct Test_Register {
  Test_Register(const char *suite, const char *name, Test_Function fn) {
    if (num_test_cases >= MAX_TEST_CASES) {
      fprintf(stderr, "more than %d test cases\n", MAX_TEST_CASES);
      exit(1);
    }
    test_cases[num_test_cases].suite = suite;
    test_cases[num_test_cases].name = name;
    test_cases[num_test_cases].fn = fn;
    num_test_cases++;
  }
};

#define TEST(SUITE, CASE) \
  static bool test_##SUITE##_##CASE(); \
  static Test_Register register_##SUITE##_##CASE(#SUITE, #CASE, test_##SUITE##_##CASE); \
  static bool test_##SUITE##_##CASE()

#define EXPECT_TRUE(COND) \
  do { \
    if (!(COND)) { \
      fprintf(stderr, "%s:%d: expected %s\n", __FILE__, __LINE__, #COND); \
      return false; \
    } \
  } while (0)

#define EXPECT_EQ(A, B) \
  do { \
    long a_ = (long)(A); \
    long b_ = (long)(B); \
    if (a_ != b_) { \
      fprintf(stderr, "%s:%d: %s is %ld, expected %ld\n", __FILE__, __LINE__, #A, a_, b_); \
      return false; \
    } \
  } while (0)

#define EXPECT_NEAR(A, B, TOL) \
  do { \
    double a_ = (A); \
    double b_ = (B); \
    if (fabs(a_ - b_) > (TOL)) { \
      fprintf(stderr, "%s:%d: %s is %g, expected %g\n", __FILE__, __LINE__, #A, a_, b_); \
      return false; \
    } \
  } while (0)

#define EXPECT_STREQ(A, B) \
  do { \
    const char *a_ = (A); \
    const char *b_ = (B); \
    if (!a_ || !b_ || strcmp(a_, b_)) { \
      fprintf(stderr, "%s:%d: %s is\n%s\nexpected\n%s\n", __FILE__, __LINE__, #A, a_, b_); \
      return false; \
    } \
  } while (0)

// ---- Fixture ---------------------------------------------------------------

static Fl_Window *win = 0;
static Fl_Dockable_Area *area = 0;
static Fl_Box *view = 0;

// An area of 1000 by 700 holding a center box
static void setup() {
  if (win) {
    Fl::do_widget_deletion();
    delete win;
    Fl::do_widget_deletion();
  }
  Fl_Group::current(0);
  win = new Fl_Window(1000, 700);
  win->end();
  Fl_Group::current(0);
  view = new Fl_Box(0, 0, 10, 10, "view");
  area = new Fl_Dockable_Area(0, 0, 1000, 700, view);
  win->add(area);
  win->resizable(area);
  win->show();
  Fl::check();
}

// A panel whose body is W by H, in a place of its own unless one is given
static Fl_Dockable_Panel *make_panel(const char *title, int W = 200, int H = 300,
                                     Fl_Dockable_Place *place = 0) {
  Fl_Dockable_Panel *p = new Fl_Dockable_Panel(area, W, H, title, place);
  p->begin_body();
  new Fl_Box(0, 0, W, H, title);
  p->end_body();
  return p;
}

// A panel that counts the application's requests
static int opens = 0;
static int closes = 0;
static int hides = 0;
class Counting_Panel : public Fl_Dockable_Panel {
public:
  Counting_Panel(const char *title, Fl_Dockable_Place *place = 0)
      : Fl_Dockable_Panel(::area, 200, 300, title, place) {}
  void open_request() override {
    opens++;
    show_panel();
  }
  void close_request() override {
    closes++;
    hide_panel();
  }
  void hide_request() override {
    hides++;
    hide_panel();
  }
};

static Fl_Dockable_Bar *bar_of(Fl_Dockable_Panel *p) {
  for (int i = 0; i < p->children(); i++) {
    Fl_Dockable_Bar *b = dynamic_cast<Fl_Dockable_Bar *>(p->child(i));
    if (b) {
      return b;
    }
  }
  return 0;
}

static int calls = 0;
static int last_reason = 0;
static Fl_Dockable_Panel *last_panel = 0;
static void area_cb(Fl_Widget *w, void *) {
  calls++;
  last_reason = Fl::callback_reason();
  last_panel = ((Fl_Dockable_Area *)w)->callback_panel();
}

// ---- Places ----------------------------------------------------------------

TEST(Place, AddAndFind) {
  setup();
  Fl_Dockable_Place *a = area->add_place("A", FL_DOCKABLE_LEFT, 10);
  EXPECT_TRUE(area->add_place("A") == a);
  EXPECT_TRUE(area->find_place("A") == a);
  EXPECT_TRUE(area->find_place("B") == 0);
  EXPECT_EQ(area->places(), 1);
  EXPECT_EQ(a->side, FL_DOCKABLE_LEFT);
  EXPECT_NEAR(a->order, 10, 0);
  // A side out of range is the right
  EXPECT_EQ(area->add_place("C", 9)->side, FL_DOCKABLE_RIGHT);
  return true;
}

TEST(Place, PanelMakesItsOwn) {
  setup();
  Fl_Dockable_Panel *p = make_panel("Files");
  EXPECT_TRUE(p->place() == area->find_place("Files"));
  EXPECT_STREQ(p->name(), "Files");
  p->name("files");
  EXPECT_STREQ(p->name(), "files");
  EXPECT_TRUE(area->find("files") == p);
  EXPECT_EQ(area->panels(), 1);
  EXPECT_EQ(p->built_w(), 200);
  EXPECT_EQ(p->built_h(), 300 + Fl_Dockable_Panel::bar_height());
  // Not in the group current as it was made
  EXPECT_TRUE(p->parent() == 0);
  return true;
}

// ---- Docking ---------------------------------------------------------------

TEST(Dock, ShowsAtRight) {
  setup();
  Fl_Dockable_Panel *p = make_panel("P");
  p->show_panel();
  EXPECT_TRUE(p->docked());
  EXPECT_TRUE(p->parent() == area);
  EXPECT_EQ(p->x(), 800);
  EXPECT_EQ(p->w(), 200);
  EXPECT_EQ(p->h(), 700);
  EXPECT_EQ(view->w(), 800);
  // The body is below the title bar, its widgets with it
  EXPECT_EQ(p->body()->y(), Fl_Dockable_Panel::bar_height());
  EXPECT_EQ(p->body()->child(0)->y(), Fl_Dockable_Panel::bar_height());
  p->hide_panel();
  EXPECT_TRUE(p->parent() == 0);
  EXPECT_EQ(view->w(), 1000);
  return true;
}

TEST(Dock, FourSides) {
  setup();
  Fl_Dockable_Panel *l = make_panel("L", 150, 100, area->add_place("L", FL_DOCKABLE_LEFT));
  Fl_Dockable_Panel *r = make_panel("R", 120, 100, area->add_place("R", FL_DOCKABLE_RIGHT));
  Fl_Dockable_Panel *t = make_panel("T", 100, 80, area->add_place("T", FL_DOCKABLE_TOP));
  Fl_Dockable_Panel *b = make_panel("B", 100, 90, area->add_place("B", FL_DOCKABLE_BOTTOM));
  l->show_panel();
  r->show_panel();
  t->show_panel();
  b->show_panel();
  EXPECT_EQ(l->x(), 0);
  EXPECT_EQ(l->w(), 150);
  EXPECT_EQ(r->x(), 1000 - 120);
  // Rows span the center's width, their own height with the title bar
  EXPECT_EQ(t->x(), 150);
  EXPECT_EQ(t->w(), 1000 - 150 - 120);
  EXPECT_EQ(t->h(), 80 + Fl_Dockable_Panel::bar_height());
  EXPECT_EQ(view->x(), 150);
  EXPECT_EQ(view->y(), t->h());
  EXPECT_EQ(view->h(), 700 - t->h() - b->h());
  EXPECT_EQ(b->y() + b->h(), 700);
  return true;
}

TEST(Dock, OrderAlongASide) {
  setup();
  Fl_Dockable_Panel *a = make_panel("A", 100, 100, area->add_place("A", FL_DOCKABLE_RIGHT, 20));
  Fl_Dockable_Panel *b = make_panel("B", 100, 100, area->add_place("B", FL_DOCKABLE_RIGHT, 10));
  a->show_panel();
  b->show_panel();
  // Smaller order further left
  EXPECT_TRUE(b->x() < a->x());
  EXPECT_EQ(a->x(), 900);
  EXPECT_EQ(b->x(), 800);
  return true;
}

TEST(Dock, FloatsWhenItDoesNotFit) {
  setup();
  // The center keeps 300 of 1000; a panel 800 wide does not fit
  Fl_Dockable_Panel *p = make_panel("Wide", 800, 100);
  p->place()->docked = true;
  EXPECT_TRUE(!area->fits(p, FL_DOCKABLE_RIGHT));
  area->min_center(100, 100);
  EXPECT_TRUE(area->fits(p, FL_DOCKABLE_RIGHT));
  return true;
}

TEST(Dock, PlacesTakeTurns) {
  setup();
  Fl_Dockable_Place *editor = area->add_place("Editor", FL_DOCKABLE_RIGHT);
  Fl_Dockable_Panel *a = make_panel("A", 200, 100, editor);
  Fl_Dockable_Panel *b = make_panel("B", 250, 100, editor);
  a->show_panel();
  b->show_panel();
  EXPECT_TRUE(a->parent() == 0);
  EXPECT_TRUE(b->docked());
  EXPECT_TRUE(editor->last == b);
  return true;
}

TEST(Dock, TabsInOnePlace) {
  setup();
  Fl_Dockable_Panel *a = make_panel("A", 200, 100, area->add_place("A", FL_DOCKABLE_RIGHT, 5));
  Fl_Dockable_Panel *b = make_panel("B", 200, 100, area->add_place("B", FL_DOCKABLE_RIGHT, 5));
  a->show_panel();
  b->show_panel();
  Fl_Dockable_Tabs *t = dynamic_cast<Fl_Dockable_Tabs *>(a->parent());
  EXPECT_TRUE(t != 0);
  EXPECT_TRUE(b->parent() == t);
  EXPECT_EQ(t->panels(), 2);
  EXPECT_TRUE(t->value() == b);
  // Showing the other brings it to the front
  a->show_panel();
  EXPECT_TRUE(t->value() == a);
  // One left alone leaves the tabs
  b->hide_panel();
  EXPECT_TRUE(a->parent() == area);
  return true;
}

TEST(Dock, StackedInOnePlace) {
  setup();
  Fl_Dockable_Place *pa = area->add_place("A", FL_DOCKABLE_RIGHT, 5);
  Fl_Dockable_Place *pb = area->add_place("B", FL_DOCKABLE_RIGHT, 5);
  pb->sub = 10;
  Fl_Dockable_Panel *a = make_panel("A", 200, 100, pa);
  Fl_Dockable_Panel *b = make_panel("B", 200, 100, pb);
  a->show_panel();
  b->show_panel();
  Fl_Dockable_Stack *s = dynamic_cast<Fl_Dockable_Stack *>(a->parent());
  EXPECT_TRUE(s != 0);
  EXPECT_TRUE(b->parent() == s);
  EXPECT_TRUE(s->is_vertical());
  EXPECT_EQ(a->y(), 0);
  EXPECT_EQ(b->y(), a->h());
  EXPECT_EQ(a->h() + b->h(), 700);
  b->hide_panel();
  EXPECT_TRUE(a->parent() == area);
  EXPECT_EQ(a->h(), 700);
  return true;
}

TEST(Dock, StackedAcrossAPart) {
  setup();
  Fl_Dockable_Place *pa = area->add_place("A", FL_DOCKABLE_RIGHT, 5);
  Fl_Dockable_Place *pb = area->add_place("B", FL_DOCKABLE_RIGHT, 5);
  Fl_Dockable_Place *pc = area->add_place("C", FL_DOCKABLE_RIGHT, 5);
  Fl_Dockable_Place *pd = area->add_place("D", FL_DOCKABLE_RIGHT, 5);
  pb->sub = 10;
  pc->cross = 5;
  pd->cross = 5;
  Fl_Dockable_Panel *a = make_panel("A", 300, 100, pa);
  Fl_Dockable_Panel *b = make_panel("B", 300, 100, pb);
  Fl_Dockable_Panel *c = make_panel("C", 300, 100, pc);
  Fl_Dockable_Panel *d = make_panel("D", 300, 100, pd);
  a->show_panel();
  b->show_panel();
  c->show_panel();
  // A column: A beside C on top, B below
  Fl_Dockable_Stack *across = dynamic_cast<Fl_Dockable_Stack *>(a->parent());
  EXPECT_TRUE(across != 0);
  EXPECT_EQ(across->level(), 1);
  EXPECT_TRUE(!across->is_vertical());
  EXPECT_TRUE(c->parent() == across);
  Fl_Dockable_Stack *along = dynamic_cast<Fl_Dockable_Stack *>(across->parent());
  EXPECT_TRUE(along != 0);
  EXPECT_EQ(along->level(), 0);
  EXPECT_TRUE(b->parent() == along);
  EXPECT_EQ(a->y(), c->y());
  EXPECT_TRUE(a->x() < c->x());
  EXPECT_EQ(a->w() + c->w(), 300);
  EXPECT_EQ(b->y(), a->h());
  // D joins C as a tab
  d->show_panel();
  Fl_Dockable_Tabs *t = dynamic_cast<Fl_Dockable_Tabs *>(d->parent());
  EXPECT_TRUE(t != 0);
  EXPECT_TRUE(c->parent() == t);
  EXPECT_TRUE(t->parent() == across);
  // Each leaving closes its part up
  d->hide_panel();
  c->hide_panel();
  EXPECT_TRUE(a->parent() == along);
  b->hide_panel();
  EXPECT_TRUE(a->parent() == area);
  EXPECT_EQ(a->h(), 700);
  return true;
}

TEST(Dock, ShrinksInASmallWindow) {
  setup();
  Fl_Dockable_Panel *l = make_panel("L", 300, 100, area->add_place("L", FL_DOCKABLE_LEFT));
  Fl_Dockable_Panel *r = make_panel("R", 300, 100, area->add_place("R", FL_DOCKABLE_RIGHT));
  l->show_panel();
  r->show_panel();
  area->resize(0, 0, 450, 700);
  // Each keeps its share, and its place keeps its width for when there is room
  EXPECT_EQ(l->w(), r->w());
  EXPECT_TRUE(l->w() + r->w() <= 450 - 50);
  EXPECT_EQ(l->place()->width, 300);
  area->resize(0, 0, 1000, 700);
  EXPECT_EQ(l->w(), 300);
  return true;
}

TEST(Dock, Maximize) {
  setup();
  Fl_Dockable_Panel *a = make_panel("A", 200, 100, area->add_place("A", FL_DOCKABLE_LEFT));
  Fl_Dockable_Panel *b = make_panel("B", 200, 100, area->add_place("B", FL_DOCKABLE_RIGHT));
  a->show_panel();
  b->show_panel();
  area->maximize(b);
  EXPECT_TRUE(area->maximized(b));
  EXPECT_EQ(b->w(), 1000);
  EXPECT_TRUE(!a->visible());
  // The center is not hidden but put out of sight
  EXPECT_TRUE(view->visible());
  area->maximize(0);
  EXPECT_TRUE(a->visible());
  EXPECT_EQ(b->w(), 200);
  EXPECT_EQ(view->w(), 600);
  return true;
}

TEST(Dock, HideDocked) {
  setup();
  opens = hides = 0;
  Counting_Panel *a = new Counting_Panel("A");
  a->show_panel();
  area->hide_docked(1);
  EXPECT_EQ(hides, 1);
  EXPECT_TRUE(a->parent() == 0);
  EXPECT_TRUE(area->docked_hidden());
  area->hide_docked(0);
  EXPECT_EQ(opens, 1);
  EXPECT_TRUE(a->docked());
  EXPECT_TRUE(!area->docked_hidden());
  return true;
}

TEST(Dock, DeletingAPanel) {
  setup();
  Fl_Dockable_Panel *a = make_panel("A");
  Fl_Dockable_Panel *b = make_panel("B");
  a->show_panel();
  b->show_panel();
  delete a;
  EXPECT_EQ(area->panels(), 1);
  EXPECT_TRUE(area->panel(0) == b);
  EXPECT_EQ(b->x(), 800);
  EXPECT_EQ(view->w(), 800);
  delete b;
  EXPECT_EQ(view->w(), 1000);
  return true;
}

TEST(Dock, DeletingTheArea) {
  setup();
  Fl_Dockable_Panel *a = make_panel("A");
  Fl_Dockable_Panel *b = make_panel("B");
  a->show_panel();
  delete win;
  win = 0;
  // The panel not docked forgets the area
  EXPECT_TRUE(b->area() == 0);
  b->show_panel();
  delete b;
  return true;
}

// ---- The user's requests ---------------------------------------------------

TEST(User, CloseButton) {
  setup();
  closes = 0;
  calls = 0;
  area->callback(area_cb);
  Counting_Panel *a = new Counting_Panel("A");
  a->show_panel();
  Fl_Dockable_Bar *bar = bar_of(a);
  EXPECT_TRUE(bar != 0);
  // A press on the close button, at the bar's right end
  Fl::e_x = bar->x() + bar->w() - 3 - (bar->h() - 6) / 2;
  Fl::e_y = bar->y() + bar->h() / 2;
  Fl::e_keysym = FL_Button + FL_LEFT_MOUSE;
  bar->handle(FL_PUSH);
  EXPECT_EQ(closes, 1);
  EXPECT_EQ(calls, 1);
  EXPECT_EQ(last_reason, FL_REASON_CLOSED);
  EXPECT_TRUE(last_panel == a);
  EXPECT_TRUE(a->parent() == 0);
  // One that cannot be closed is not, from the window it floats in either
  a->show_panel();
  a->dock_flags(FL_DOCKABLE_MOVABLE | FL_DOCKABLE_FLOATABLE);
  a->float_panel(100, 100, 0);
  a->window()->do_callback();
  EXPECT_EQ(closes, 1);
  EXPECT_TRUE(a->panel_shown());
  a->dock_flags(FL_DOCKABLE_DEFAULT_FLAGS);
  a->window()->do_callback();
  EXPECT_EQ(closes, 2);
  EXPECT_TRUE(!a->panel_shown());
  area->callback((Fl_Callback *)0);
  return true;
}

TEST(User, Shortcut) {
  setup();
  opens = hides = 0;
  Counting_Panel *a = new Counting_Panel("A");
  a->dock_shortcut(FL_ALT + 'p');
  Fl::e_keysym = 'p';
  Fl::e_state = FL_ALT;
  static char text[2] = "p";
  Fl::e_text = text;
  Fl::e_length = 1;
  EXPECT_TRUE(area->key_pressed());
  EXPECT_EQ(opens, 1);
  EXPECT_TRUE(a->docked());
  Fl::e_keysym = 'q';
  text[0] = 'q';
  EXPECT_TRUE(!area->key_pressed());
  Fl::e_state = 0;
  return true;
}

TEST(User, PanelMenu) {
  setup();
  Fl_Group::current(0);
  Fl_Menu_Bar *mb = new Fl_Menu_Bar(0, 0, 100, 20);
  mb->add("Window/Reset", 0, 0);
  area->panel_menu(mb, "Window/Panels");
  Fl_Dockable_Panel *b = make_panel("B/b");
  Fl_Dockable_Panel *a = make_panel("A");
  // The list follows the panels at the next turn of the event loop
  Fl::check();
  // Listed by title, the slash in a title kept
  const Fl_Menu_Item *m = mb->find_item("Window/Panels");
  EXPECT_TRUE(m != 0);
  m++;
  EXPECT_STREQ(m[0].label(), "A");
  EXPECT_STREQ(m[1].label(), "B/b");
  EXPECT_TRUE(m[0].user_data() == a);
  EXPECT_TRUE(m[1].user_data() == b);
  EXPECT_TRUE(!m[0].value());
  a->show_panel();
  Fl::check();
  m = mb->find_item("Window/Panels") + 1;
  EXPECT_TRUE(m[0].value());
  area->panel_menu(0, 0);
  delete mb;
  return true;
}

// A panel whose close request deletes it
class Deleting_Panel : public Fl_Dockable_Panel {
public:
  Deleting_Panel(const char *title) : Fl_Dockable_Panel(::area, 200, 300, title) {}
  void close_request() override { delete this; }
};

// ---- Lifetimes -------------------------------------------------------------

TEST(Life, ConstructorKeepsTheCurrentGroup) {
  setup();
  Fl_Group::current(0);
  Fl_Window *w2 = new Fl_Window(300, 300);
  Fl_Dockable_Panel *q = new Fl_Dockable_Panel(area, 200, 300, "Q");
  EXPECT_TRUE(Fl_Group::current() == w2);
  Fl_Box *b = new Fl_Box(0, 0, 10, 10, "after");
  EXPECT_TRUE(b->parent() == w2);
  q->begin_body();
  Fl_Box *in = new Fl_Box(0, 0, 10, 10, "in");
  q->end_body();
  EXPECT_TRUE(in->parent() == q->body());
  EXPECT_TRUE(Fl_Group::current() == w2);
  w2->end();
  EXPECT_TRUE(q->parent() == 0);
  delete w2;
  return true;
}

TEST(Life, SameTitleTwoPlaces) {
  setup();
  Fl_Dockable_Panel *a = make_panel("Same");
  Fl_Dockable_Panel *b = make_panel("Same");
  EXPECT_TRUE(a->place() != b->place());
  a->show_panel();
  b->show_panel();
  EXPECT_TRUE(a->docked());
  EXPECT_TRUE(b->docked());
  return true;
}

TEST(Life, FloatingPanelOutlivesTheArea) {
  setup();
  Fl_Dockable_Panel *b = make_panel("B");
  b->float_as(0);
  Fl::check();
  EXPECT_TRUE(b->panel_shown());
  delete win;
  win = 0;
  EXPECT_TRUE(b->area() == 0);
  EXPECT_STREQ(b->place()->name.c_str(), "B");
  // What its menu would do, now nothing
  b->dock_on(FL_DOCKABLE_LEFT);
  b->float_as(1);
  b->auto_hide();
  b->hide_panel();
  delete b;
  return true;
}

TEST(Life, MenuOutlivesTheArea) {
  setup();
  Fl_Group::current(0);
  Fl_Window *mw = new Fl_Window(300, 40);
  Fl_Menu_Bar *mb = new Fl_Menu_Bar(0, 0, 300, 25);
  mw->end();
  area->panel_menu(mb, "Window/Panels");
  Fl_Dockable_Panel *a = make_panel("A");
  a->show_panel();
  Fl::check();
  EXPECT_TRUE(mb->find_item("Window/Panels/A") != 0);
  delete win;
  win = 0;
  Fl::do_widget_deletion();
  EXPECT_TRUE(mb->find_item("Window/Panels/A") == 0);
  delete mw;
  return true;
}

TEST(Life, MenuDeletedFirst) {
  setup();
  Fl_Group::current(0);
  Fl_Menu_Bar *mb = new Fl_Menu_Bar(0, 0, 300, 25);
  area->panel_menu(mb, "Window/Panels");
  Fl_Dockable_Panel *a = make_panel("A");
  delete mb;
  a->show_panel();
  Fl::check();
  EXPECT_TRUE(a->docked());
  return true;
}

TEST(Life, CloseRequestDeletesThePanel) {
  setup();
  calls = 0;
  area->callback(area_cb);
  Deleting_Panel *d = new Deleting_Panel("D");
  d->show_panel();
  Fl_Dockable_Bar *bar = bar_of(d);
  Fl::e_x = bar->x() + bar->w() - 3 - (bar->h() - 6) / 2;
  Fl::e_y = bar->y() + bar->h() / 2;
  Fl::e_keysym = FL_Button + FL_LEFT_MOUSE;
  bar->handle(FL_PUSH);
  EXPECT_EQ(area->panels(), 0);
  EXPECT_EQ(calls, 1);
  EXPECT_TRUE(last_panel == 0);
  EXPECT_EQ(view->w(), 1000);
  area->callback((Fl_Callback *)0);
  return true;
}

TEST(Life, HugeOrdersDoNotHang) {
  setup();
  Fl_Dockable_Panel *a = make_panel("A");
  Fl_Dockable_Panel *b = make_panel("B");
  area->read_layout("place A side=0 order=1e20\nplace B side=1 order=nan\n");
  // Out of range, they keep the places' own
  EXPECT_NEAR(a->place()->order, 0, 0);
  EXPECT_NEAR(b->place()->order, 0, 0);
  a->place()->order = 5e8;
  a->show_panel();
  b->show_panel();
  b->dock_on(FL_DOCKABLE_LEFT);
  EXPECT_TRUE(b->docked());
  return true;
}

// ---- Layouts ---------------------------------------------------------------

TEST(Layout, TextRoundTrip) {
  setup();
  Fl_Dockable_Place *p = area->add_place("Tool box", FL_DOCKABLE_LEFT, 3);
  Fl_Dockable_Panel *a = make_panel("A", 200, 100, p);
  area->save_defaults();
  p->side = FL_DOCKABLE_BOTTOM;
  p->order = 7.5;
  p->height = 120;
  p->stripe = FL_DOCKABLE_TOP;
  p->auto_hide = true;
  p->float_set = true;
  p->float_x = 40;
  p->float_y = 50;
  a->float_size(300, 400);
  char buf[2048];
  int n = area->write_layout(buf, sizeof(buf));
  EXPECT_EQ(n, (int)strlen(buf));
  EXPECT_TRUE(strstr(buf, "place Tool%20box ") != 0);
  area->reset_layout();
  EXPECT_EQ(p->side, FL_DOCKABLE_LEFT);
  EXPECT_EQ(a->float_w(), 0);
  EXPECT_TRUE(area->read_layout(buf));
  EXPECT_EQ(p->side, FL_DOCKABLE_BOTTOM);
  EXPECT_NEAR(p->order, 7.5, 1e-9);
  EXPECT_EQ(p->height, 120);
  EXPECT_EQ(p->stripe, FL_DOCKABLE_TOP);
  EXPECT_TRUE(p->auto_hide);
  EXPECT_EQ(p->float_x, 40);
  EXPECT_EQ(a->float_w(), 300);
  EXPECT_EQ(a->float_h(), 400);
  // Too small a buffer gets what fits and the whole length
  char small[16];
  EXPECT_EQ(area->write_layout(small, sizeof(small)), n);
  EXPECT_EQ((int)strlen(small), 15);
  return true;
}

TEST(Layout, BadValuesKeepThePlaces) {
  setup();
  Fl_Dockable_Place *p = area->add_place("P", FL_DOCKABLE_LEFT, 3);
  EXPECT_TRUE(area->read_layout("place P side=9 stripe=-4 autohide=1 width=-5 x=12 junk\n"
                                "nonsense line\nplace Unknown side=1\nsize Nobody 3 4\n"));
  EXPECT_EQ(p->side, FL_DOCKABLE_LEFT);
  EXPECT_EQ(p->stripe, -1);
  // Without a stripe it cannot auto-hide
  EXPECT_TRUE(!p->auto_hide);
  EXPECT_EQ(p->width, 0);
  EXPECT_EQ(p->float_x, 12);
  EXPECT_TRUE(!area->read_layout("place Unknown side=1\n"));
  return true;
}

TEST(Layout, Preferences) {
  setup();
  Fl_Dockable_Place *p = area->add_place("P", FL_DOCKABLE_LEFT, 3);
  Fl_Dockable_Panel *a = make_panel("A", 200, 100, p);
  a->float_size(320, 240);
  p->width = 260;
  Fl_Preferences prefs(Fl_Preferences::MEMORY, "fltk.org", "unittest_dockable");
  EXPECT_TRUE(area->save_layout(prefs));
  EXPECT_TRUE(area->save_layout(prefs, "Wide"));
  p->width = 0;
  a->float_size(0, 0);
  EXPECT_TRUE(area->load_layout(prefs));
  EXPECT_EQ(p->width, 260);
  EXPECT_EQ(a->float_w(), 320);
  p->width = 0;
  EXPECT_TRUE(area->load_layout(prefs, "Wide"));
  EXPECT_EQ(p->width, 260);
  EXPECT_TRUE(!area->load_layout(prefs, "Narrow"));
  return true;
}

TEST(Layout, ResetShowsThroughTheApplication) {
  setup();
  opens = hides = 0;
  Fl_Dockable_Place *p = area->add_place("P", FL_DOCKABLE_RIGHT, 3);
  Counting_Panel *a = new Counting_Panel("A", p);
  area->save_defaults();
  a->show_panel();
  a->dock_on(FL_DOCKABLE_LEFT);
  EXPECT_EQ(a->x(), 0);
  area->reset_layout();
  EXPECT_EQ(hides, 1);
  EXPECT_EQ(opens, 1);
  EXPECT_EQ(p->side, FL_DOCKABLE_RIGHT);
  EXPECT_EQ(a->x(), 800);
  return true;
}

TEST(Layout, ShowSaved) {
  setup();
  opens = 0;
  Fl_Dockable_Place *p = area->add_place("P", FL_DOCKABLE_RIGHT, 3);
  Fl_Dockable_Place *q = area->add_place("Q", FL_DOCKABLE_RIGHT, 4);
  new Counting_Panel("A", p);
  EXPECT_TRUE(area->read_layout("place P shown=1\nplace Q shown=1\n"));
  area->show_saved();
  // Q has no panel, and no factory makes one
  EXPECT_EQ(opens, 1);
  (void)q;
  return true;
}

static Fl_Dockable_Panel *factory(Fl_Dockable_Area *a, Fl_Dockable_Place *p, void *data) {
  (*(int *)data)++;
  return new Counting_Panel(p->name.c_str(), p);
}

TEST(Layout, Factory) {
  setup();
  int made = 0;
  area->panel_factory(factory, &made);
  Fl_Dockable_Place *p = area->add_place("Later", FL_DOCKABLE_RIGHT, 3);
  EXPECT_TRUE(area->show_place(p));
  EXPECT_EQ(made, 1);
  EXPECT_TRUE(area->find("Later") != 0);
  EXPECT_TRUE(area->find("Later")->docked());
  // Shown again, it is not made again
  area->find("Later")->hide_panel();
  EXPECT_TRUE(area->show_place(p));
  EXPECT_EQ(made, 1);
  return true;
}

// ---- The area's settings ---------------------------------------------------

TEST(Area, CenterAndVersion) {
  setup();
  EXPECT_TRUE(area->center() == view);
  EXPECT_EQ(Fl_Dockable_Area::api_version(), FL_DOCKABLE_AREA_API_VERSION);
  EXPECT_EQ(FL_DOCKABLE_AREA_API_VERSION, FL_DOCKABLE_AREA_MAJOR_VERSION * 10000 +
                                              FL_DOCKABLE_AREA_MINOR_VERSION * 100 +
                                              FL_DOCKABLE_AREA_PATCH_VERSION);
  return true;
}

TEST(Area, Settings) {
  setup();
  area->min_center(120, 90);
  EXPECT_EQ(area->min_center_w(), 120);
  EXPECT_EQ(area->min_center_h(), 90);
  area->dock_band(30);
  EXPECT_EQ(area->dock_band(), 30);
  EXPECT_TRUE(Fl_Dockable_Area::stripe_thickness() > 0);
  return true;
}

TEST(Area, RelayoutAfterAPlaceChanges) {
  setup();
  Fl_Dockable_Panel *p = make_panel("P");
  p->show_panel();
  EXPECT_EQ(p->w(), 200);
  p->place()->width = 300;
  area->relayout();
  EXPECT_EQ(p->w(), 300);
  EXPECT_EQ(view->w(), 700);
  return true;
}

// ---- Stripes -----------------------------------------------------------------

TEST(Stripe, ButtonTakesRoom) {
  setup();
  int t = Fl_Dockable_Area::stripe_thickness();
  Fl_Dockable_Panel *p = make_panel("P");
  p->show_panel();
  p->show_button(1);
  EXPECT_EQ(p->place()->stripe, FL_DOCKABLE_RIGHT);
  EXPECT_EQ(p->x() + p->w(), 1000 - t);
  EXPECT_EQ(view->w(), 1000 - 200 - t);
  p->show_button(0);
  EXPECT_EQ(p->place()->stripe, -1);
  EXPECT_EQ(view->w(), 800);
  return true;
}

TEST(Stripe, ShowEmpty) {
  setup();
  int t = Fl_Dockable_Area::stripe_thickness();
  area->stripe_options(FL_DOCKABLE_STRIPE_SHOW_EMPTY);
  EXPECT_EQ(area->stripe_options(), FL_DOCKABLE_STRIPE_SHOW_EMPTY);
  EXPECT_EQ(view->x(), t);
  EXPECT_EQ(view->w(), 1000 - 2 * t);
  EXPECT_EQ(view->h(), 700 - 2 * t);
  area->stripe_options(0);
  EXPECT_EQ(view->w(), 1000);
  return true;
}

TEST(Stripe, AutoHideSlidesOutAndAway) {
  setup();
  Fl_Dockable_Layer *layer = new Fl_Dockable_Layer(0, 0, 1000, 700);
  win->add(layer);
  area->captive_layer(layer);
  EXPECT_TRUE(area->captive_layer() == layer);
  Fl_Dockable_Panel *p = make_panel("P");
  p->show_panel();
  p->auto_hide();
  EXPECT_TRUE(p->place()->auto_hide);
  EXPECT_EQ(p->place()->stripe, FL_DOCKABLE_RIGHT);
  EXPECT_TRUE(!p->panel_shown());
  // Shown, it slides out over the center in a window of the layer
  p->show_panel();
  Fl::check();
  EXPECT_TRUE(p->panel_shown());
  EXPECT_TRUE(!p->docked());
  EXPECT_TRUE(p->window()->parent() == layer);
  // A click away from it slides it away
  Fl::e_x_root = win->x_root() + 100;
  Fl::e_y_root = win->y_root() + 100;
  Fl::e_x = 100;
  Fl::e_y = 100;
  Fl_Dockable_Area::dispatch(FL_PUSH, win);
  Fl_Dockable_Area::dispatch(FL_RELEASE, win);
  Fl::check();
  EXPECT_TRUE(!p->panel_shown());
  // Pinned, it docks where it was
  p->pin();
  EXPECT_TRUE(!p->place()->auto_hide);
  EXPECT_EQ(p->place()->stripe, FL_DOCKABLE_RIGHT);
  p->show_panel();
  EXPECT_TRUE(p->docked());
  return true;
}

TEST(Stripe, Icon) {
  setup();
  static unsigned char pixels[16 * 16 * 3];
  Fl_RGB_Image icon(pixels, 16, 16, 3);
  Fl_Dockable_Panel *p = make_panel("P");
  EXPECT_TRUE(p->dock_icon() == 0);
  p->dock_icon(&icon);
  EXPECT_TRUE(p->dock_icon() == &icon);
  p->show_panel();
  p->show_button(1);
  Fl::check();
  p->dock_icon(0);
  return true;
}

// ---- Panels ------------------------------------------------------------------

TEST(Panel, Title) {
  setup();
  Fl_Dockable_Panel *p = make_panel("P");
  p->name("keep");
  p->title("Renamed");
  EXPECT_STREQ(p->title(), "Renamed");
  EXPECT_STREQ(p->label(), "Renamed");
  EXPECT_STREQ(p->name(), "keep");
  p->float_panel(100, 100, 0);
  p->title("Floating");
  EXPECT_STREQ(p->window()->label(), "Floating");
  return true;
}

TEST(Panel, DockAFloatingPanel) {
  setup();
  Fl_Dockable_Panel *p = make_panel("P");
  p->float_panel(100, 100, 0);
  EXPECT_TRUE(p->panel_shown());
  EXPECT_TRUE(!p->docked());
  p->dock_panel();
  Fl::do_widget_deletion();
  EXPECT_TRUE(p->docked());
  EXPECT_TRUE(p->place()->docked);
  EXPECT_TRUE(p->window() == win);
  return true;
}

TEST(Panel, AdoptLendAndTakeBack) {
  setup();
  Fl_Group::current(0);
  Fl_Window *built = new Fl_Window(150, 120);
  Fl_Box *box = new Fl_Box(10, 20, 50, 30, "box");
  built->end();
  Fl_Dockable_Panel *p = new Fl_Dockable_Panel(area, 10, 10, "A");
  p->adopt(built);
  EXPECT_EQ(built->children(), 0);
  EXPECT_TRUE(box->parent() == p->body());
  EXPECT_EQ(p->body()->w(), 150);
  EXPECT_EQ(p->built_w(), 150);
  EXPECT_EQ(p->built_h(), 120 + Fl_Dockable_Panel::bar_height());
  p->show_panel();
  int bx = box->x() - p->body()->x();
  int by = box->y() - p->body()->y();
  int bw = box->w();
  int bh = box->h();
  // Lent back to the window, the widgets are placed as they were built
  p->lend_body(built);
  EXPECT_TRUE(box->parent() == built);
  EXPECT_EQ(box->x(), 10);
  EXPECT_EQ(box->y(), 20);
  EXPECT_EQ(box->w(), 50);
  EXPECT_EQ(box->h(), 30);
  // Taken back, they are where they were in the panel
  p->take_back_body(built);
  EXPECT_TRUE(box->parent() == p->body());
  EXPECT_EQ(box->x() - p->body()->x(), bx);
  EXPECT_EQ(box->y() - p->body()->y(), by);
  EXPECT_EQ(box->w(), bw);
  EXPECT_EQ(box->h(), bh);
  delete built;
  return true;
}

static int resizes = 0;
class Resize_Panel : public Fl_Dockable_Panel {
public:
  Resize_Panel() : Fl_Dockable_Panel(::area, 200, 300, "R") {}
  void body_resized() override { resizes++; }
};

TEST(Panel, BodyResized) {
  setup();
  Resize_Panel *p = new Resize_Panel;
  p->show_panel();
  resizes = 0;
  win->size(1000, 600);
  Fl::check();
  EXPECT_TRUE(resizes > 0);
  EXPECT_EQ(p->h(), 600);
  return true;
}

// ---- Main ------------------------------------------------------------------

int main(int argc, char **argv) {
  setvbuf(stdout, 0, _IONBF, 0);
  fl_open_display();
  int failed = 0;
  for (int i = 0; i < num_test_cases; i++) {
    Test_Case &c = test_cases[i];
    if (argc > 1 && strcmp(argv[1], c.suite)) {
      continue;
    }
    bool ok = c.fn();
    if (ok) {
      printf("ok     %s.%s\n", c.suite, c.name);
    } else {
      printf("FAILED %s.%s\n", c.suite, c.name);
      failed++;
    }
  }
  if (win) {
    delete win;
  }
  printf("%d of %d failed\n", failed, num_test_cases);
  return failed > 0;
}
