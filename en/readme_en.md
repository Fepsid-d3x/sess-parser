# sess.hpp

Single-header C++17 style sheet parser for game and application UI.

SESS is a simplified CSS-like format. Instead of hardcoding colors, sizes, and positions directly in code, you describe the appearance of UI elements in `.sess` files and load them at runtime. A designer edits the file - the C++ code stays untouched.

```css
@set accent = #89b4faff;

button {
    background-color: @accent;
    width: 120;
    height: 36;
    border-radius: 6;
    anchor: center;
}

button:hover {
    background-color: #cba6f7ff;
}
```

---

## Features

- **Selectors** - by tag, class `.`, id `#`, and combinations
- **States** - `:hover`, `:active`, `:focus`, `:disabled`, `:checked`
- **Value types** - color (hex, rgb/rgba, named), number, string, bool
- **Variables** - `@set name = value` and substitution `@name`
- **`@include`** - split styles across multiple files
- **Resolve** - cascaded style merging (tag → class → id → state)
- **Async loading** - load in a background thread
- **No dependencies** - single `.hpp` file, C++17, STL only

---

## Setup

Copy `sess.hpp` into your project and include it:

```cpp
#include "sess.hpp"
using namespace fd::sess;
```

---

## Examples

### Loading a file

```cpp
SESS sheet;
sheet.load("assets/ui/main.sess");

if (!sheet.ok()) {
    // file not found or failed to open //
}
```

### Direct selector lookup

Returns the style of the first match - no cascade.

```cpp
auto s = sheet["button"];

if (s.has("background-color"))
    widget.bg = s.color("background-color");

if (s.has("font-size"))
    label.font_size = s.number("font-size");

if (s.has("visible"))
    widget.visible = s.boolean("visible");

if (s.has("font-family"))
    label.font = s.string("font-family");

// with fallback values //
float w = s.number("width", 100.f);
SESSColor c = s.color("color", {255, 255, 255});
```

### Loading from a string

Useful for tests or embedded styles:

```cpp
const char* src = R"(
    @set bg = #1e1e2e;

    panel {
        background-color: @bg;
        border-radius: 8;
    }
)";

SESS sheet;
sheet.load_from_string(src);
```

### Resolve — cascaded lookup

`resolve` merges styles by CSS priority: tag -> tag state -> classes -> id.  
Use it when an element has multiple classes or an id.

```cpp
// sheet.resolve(tag, id, {classes}, state) ..
auto s = sheet.resolve("button", "play", {".menu-button", ".large"}, SESSState::Hover);

widget.x      = s.number("x");
widget.y      = s.number("y");
widget.width  = s.number("width", 120.f);
widget.height = s.number("height", 36.f);
widget.bg     = s.color("background-color");
widget.anchor = s.anchor("anchor");
widget.pivot  = s.pivot("pivot");
```

`.sess` file for this example:

```css
button {
    background-color: #313244;
    color: #cdd6f4;
    width: 120;
    height: 36;
}

button:hover {
    background-color: #45475a;
}

.menu-button {
    border-radius: 6;
    padding: 8;
}

.large {
    height: 48;
    font-size: 16;
}

#play {
    x: 640;
    y: 360;
    anchor: center;
    pivot: center;
}
```

### States

```cpp
// derive state from widget status //
SESSState state = SESSState::None;
if (button.is_hovered()) state = SESSState::Hover;
if (button.is_pressed()) state = SESSState::Active;

auto s = sheet.resolve("button", "", {".menu-button"}, state);
widget.bg = s.color("background-color");
```

States can be combined with `|`:

```cpp
auto s = sheet["button:hover"];
// or via resolve with an explicit state //
auto s = sheet.resolve("button", {}, {}, SESSState::Hover | SESSState::Focused);
```

### Async loading

```cpp
// start loading in the background //
auto handle = sess::load_async("assets/ui/main.sess");

// do other work in parallel... //
init_renderer();
load_textures();

// block until done //
handle.wait();

if (handle.data.ok()) {
    auto s = handle.data["button"];
    // ... //
}
```

Non-blocking check:

```cpp
if (handle.done()) {
    // loading is complete //
}
```

### Variables and @include

`theme/colors.sess`:
```css
@set base    = #1e1e2eff;
@set surface = #313244ff;
@set text    = #cdd6f4ff;
@set accent  = #89b4faff;
```

`ui/buttons.sess`:
```css
@include "../theme/colors.sess"

button {
    background-color: @surface;
    color: @text;
}

button:hover {
    background-color: @accent;
    color: @base;
}
```

`ui/main.sess`:
```css
@include "buttons.sess"
@include "panels.sess"
@include "inputs.sess"
```

```cpp
SESS sheet;
sheet.load("ui/main.sess"); // pulls in all @includes automatically //
```

### Free functions

```cpp
// synchronous load //
SESS sheet = sess::load("ui/main.sess");

// load from string //
SESS sheet = sess::from_string(R"(
    button { background: #333; }
)");

// async //
SESSResult handle = sess::load_async("ui/main.sess");
handle.wait();
```

---

## Value types in C++

### `SESSColor`

```cpp
SESSColor c = style.color("background-color");

c.r; c.g; c.b; c.a;             // uint8_t, 0–255  //
c.rf(); c.gf(); c.bf(); c.af(); // float, 0.0–1.0 //

c.to_rgba(); // uint32_t, RRGGBBAA                                         //
c.to_argb(); // uint32_t, AARRGGBB (Windows GDI / Direct2D format)  grrrrr //
```

### `SESSValue`

Universal type for any property:

```cpp
const SESSValue& v = style.get("width");

v.is_number(); // float //
v.is_color();  // SESSColor //
v.is_string(); // std::string //
v.is_bool();   // bool //
v.is_none();   // property not set //

v.number(0.f);         // float with fallback //
v.integer(0);          // int with fallback //
v.color({});           // SESSColor with fallback //
v.boolean(false);      // bool with fallback //
v.string_ref();        // const std::string& //
v.string_view();       // std::string_view //
```

### `UIAnchor` / `UIPivot`

```cpp
UIAnchor a = style.anchor("anchor", UIAnchor::TopLeft);
UIPivot  p = style.pivot("pivot",   UIPivot::Center);

// values: TopLeft, Top, TopRight,           //
//         Left, Center, Right,              //
//         BottomLeft, Bottom, BottomRight   //
```

---

## States (`SESSState`)

| Value               | `.sess` syntax |
|---------------------|----------------|
| `SESSState::None`     | —              |
| `SESSState::Hover`    | `:hover`       |
| `SESSState::Active`   | `:active`      |
| `SESSState::Focused`  | `:focus`       |
| `SESSState::Disabled` | `:disabled`    |
| `SESSState::Checked`  | `:checked`     |

---

## Requirements

- **C++17** or later
- Standard library (STL)
- No external dependencies