# SESS Syntax Reference

SESS is a simplified CSS-like format for describing UI styles in games and applications.  
Files use the `.sess` extension.

---

## Comments

```css
// single-line comment

/* multi-line
   comment */
```

---

## Variables (`@set`)

Declared anywhere in the file before use.  
The value can be a color, number, or string.  
A trailing semicolon is optional.

```css
@set surface    = #89b4faff;
@set on-surface = #cdd6f4ff;
@set radius     = 6;
@set font-main  = Roboto;
```

Substitute a variable with `@name`:

```css
button {
    background-color: @surface;
    color: @on-surface;
    border-radius: @radius;
    font-family: @font-main;
}
```

Variables can reference each other:

```css
@set base-pad  = 8;
@set inner-pad = @base-pad;
```

---

## File inclusion (`@include`)

Includes another `.sess` file. The path is relative to the current file.  
Nesting up to 8 levels deep is supported.

```css
@include "theme/colors.sess"
@include "theme/typography.sess"
@include "components/buttons.sess"
```

Typical project structure:

```
ui/
├── main.sess           ← entry point
├── theme/
│   ├── colors.sess     ← @set color variables
│   └── typography.sess ← @set fonts, sizes
└── components/
    ├── buttons.sess
    ├── panels.sess
    └── inputs.sess
```

`main.sess`:
```css
@include "theme/colors.sess"
@include "theme/typography.sess"
@include "components/buttons.sess"
@include "components/panels.sess"
```

---

## Rules

General syntax:

```
selector {
    property: value;
    property: value;
}
```

---

## Selectors

### By tag

```css
button {
    background-color: #303030;
}

label {
    font-size: 14;
}
```

### By class (`.`)

```css
.menu-button {
    padding: 8;
    border-radius: 4;
}

.icon-button {
    width: 32;
    height: 32;
}
```

### By ID (`#`)

```css
#play-button {
    x: 100;
    y: 200;
}

#main-panel {
    width: 400;
    height: 300;
}
```

### Combined

Tag, classes, and ID can be combined in any order:

```css
button.menu-button {
    font-size: 15;
}

button#play-button {
    background-color: #4caf50;
}

button.menu-button.large {
    height: 48;
}
```

### Grouping (comma)

```css
button, .clickable, #submit {
    cursor: pointer;
}
```

---

## States

A state is appended with `:` after the selector.

| State      | Syntax      |
|------------|-------------|
| Hover      | `:hover`    |
| Press      | `:active`   |
| Focus      | `:focus`    |
| Disabled   | `:disabled` |
| Checked    | `:checked`  |

```css
button:hover {
    background-color: #404040;
}

button:active {
    background-color: #202020;
}

button:disabled {
    color: #666666;
    background-color: #1a1a1a;
}

.checkbox:checked {
    background-color: @accent;
}

input:focus {
    border-color: @accent;
}
```

States work with all selector types:

```css
button.menu-button:hover {
    background-color: @surface-hover;
}

#play-button:active {
    scale: 0.95;
}
```

---

## Value types

### Numbers

Numbers without units, with `px`, `em`, or `%` — all parsed as `float`.

```css
element {
    width: 128;
    height: 32px;
    opacity: 0.75;
    font-size: 16em;
    margin: 50%;
}
```

### Colors

Supported formats:

```css
element {
    /* HEX */
    color: #fff;             /* #RGB  → fully opaque */
    color: #fffc;            /* #RGBA */
    color: #ffffff;          /* #RRGGBB */
    color: #ffffffff;        /* #RRGGBBAA */

    /* RGB / RGBA functions */
    color: rgb(255, 128, 0);
    color: rgba(255, 128, 0, 0.5);

    /* Named colors */
    color: black;
    color: white;
    color: red;
    color: green;
    color: blue;
    color: yellow;
    color: cyan;
    color: magenta;
    color: orange;
    color: purple;
    color: pink;
    color: gray;
    color: transparent;
}
```

### Booleans

```css
element {
    visible: true;
    enabled: false;
    clip: yes;
    shadow: no;
}
```

### Strings

Unquoted or quoted strings:

```css
element {
    font-family: Roboto;
    text: "Hello, World!";
    align: center;
}
```

### Anchor and Pivot

Special string values for element positioning:

```css
element {
    anchor: top-left;      /* or topleft */
    anchor: top;
    anchor: top-right;
    anchor: left;
    anchor: center;        /* or middle */
    anchor: right;
    anchor: bottom-left;
    anchor: bottom;
    anchor: bottom-right;

    pivot: center;
}
```

---

## Full example

`theme/colors.sess`:
```css
// Catppuccin Mocha palette
@set base     = #1e1e2eff;
@set mantle   = #181825ff;
@set surface0 = #313244ff;
@set surface1 = #45475aff;
@set text     = #cdd6f4ff;
@set subtext  = #a6adc8ff;
@set accent   = #89b4faff;
@set red      = #f38ba8ff;
@set green    = #a6e3a1ff;
```

`components/buttons.sess`:
```css
@include "../theme/colors.sess"

button {
    width: 120;
    height: 36;
    background-color: @surface0;
    color: @text;
    font-size: 14;
    border-radius: 6;
    anchor: top-left;
    pivot: center;
}

button:hover {
    background-color: @surface1;
}

button:active {
    background-color: @base;
}

button:disabled {
    color: @subtext;
    background-color: @mantle;
}

button.primary {
    background-color: @accent;
    color: @base;
}

button.primary:hover {
    background-color: @text;
}

button.danger {
    background-color: @red;
    color: @base;
}

#confirm-button {
    width: 160;
}
```