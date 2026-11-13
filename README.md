# Fl_Dockable_Area

Docking panels for the Fast Light Tool Kit (FLTK), built on FLTK's
`Fl_Dockable_Group`.

An `Fl_Dockable_Area` holds a center widget, such as a 3D view, and docks
`Fl_Dockable_Panel` groups along its four borders: columns at the left and
right, rows at the top and bottom.  The user drags a panel by its title bar:

- over the docked panels, or a band of the center along a border, a
  placeholder shows where it will dock;
- over the middle of a docked panel it joins it as a tab; over its ends it
  stacks before or after it in that place; over the sides of a part of a
  stack it stacks across that part;
- anywhere else, or with Ctrl held, it floats: inside the application's
  window, over the area, or in a window of its own outside it.

Each panel's title bar has a menu: dock it along any border, float it inside
or in a window of its own, auto-hide it on a stripe, give it a stripe button,
maximize it, or close it.  Stripes along the borders hold a button for each
panel given one; an auto-hidden panel slides out from its button and slides
away when the user clicks elsewhere or presses Escape.  Docked panels keep the
sizes the user gives them, and shrink in proportion in a window too small for
them.  The layout is saved to and read from `Fl_Preferences`, under a name or
not, or as text, and can be reset to the application's default.

Dragging a panel out of its window and back is `Fl_Dockable_Group`'s, so it
works wherever FLTK's does, Wayland included.  This library is the layer
between it and an application: it decides where panels go.

## Requirements

- FLTK 1.5 or later with `Fl_Dockable_Group` and `Fl_Dockable_Group::captive()`,
  which are not yet part of an FLTK release; the `dock-undock` branch of
  https://github.com/ramcdona/fltk has them.
- CMake 3.15 or later.
- A C++11 compiler.

## Building

```
cmake -S . -B build -D CMAKE_PREFIX_PATH=<FLTK install>
cmake --build build
ctest --test-dir build
cmake --install build
```

The tests open windows, so they need a display.  The options
`FL_DOCKABLE_AREA_BUILD_TEST` (on), `FL_DOCKABLE_AREA_BUILD_HTML_DOCS` and
`FL_DOCKABLE_AREA_INSTALL_HTML_DOCS` (off) choose the tests and the demo,
`test/dockable_area.cxx`, and the manual, built with Doxygen by the target
`html`.  Another project finds the installed library with
`find_package(Fl_Dockable_Area CONFIG)` and links `fltk::dockable_area`.

## Use

```
Fl_Dockable_Area *area = new Fl_Dockable_Area(0, 25, 1200, 775, view);
window->add(area);
Fl_Dockable_Layer *layer = new Fl_Dockable_Layer(0, 25, 1200, 775);
window->add(layer);          // panels float captive over the area here
area->captive_layer(layer);

Fl_Dockable_Place *files = area->add_place("Files", FL_DOCKABLE_LEFT, 0);
Fl_Dockable_Panel *p = new Fl_Dockable_Panel(area, 220, 400, "Files", files);
p->begin_body();
new Fl_Browser(0, 0, 220, 400);
p->end_body();

area->save_defaults();       // the default layout, for reset_layout()
area->load_layout(prefs);    // the user's
area->callback(save_cb);     // called whenever the user changes the layout
window->show();
p->show_panel();
```

A place says where its panels show; panels that take turns in one place, as
the editors of different kinds of object might, share one.  An application
runs its own code when the user opens, hides or closes a panel by overriding
`Fl_Dockable_Panel::open_request()`, `hide_request()` and `close_request()`.

The manual, `documentation/src`, describes every feature and how the library
works; CHANGES.txt lists what each release changed.

## License

The FLTK license, the GNU Library General Public License with exceptions;
see COPYING.
