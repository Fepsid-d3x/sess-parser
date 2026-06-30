# sess.hpp

Single-header C++17 парсер стилей для UI в играх и приложениях.

SESS - упрощённый CSS-подобный формат. Вместо того чтобы хардкодить цвета, размеры и позиции прямо в коде, вы описываете внешний вид элементов в `.sess` файлах и читаете их во время выполнения. Дизайнер правит файл — C++ код не трогается.

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

## Возможности

- **Селекторы** - по тегу, классу `.`, идентификатору `#`, их комбинации
- **Состояния** - `:hover`, `:active`, `:focus`, `:disabled`, `:checked`
- **Типы значений** - цвет (hex, rgb/rgba, named), число, строка, bool
- **Переменные** - `@set name = value` и подстановка `@name`
- **`@include`** - разбивка стилей на несколько файлов
- **Resolve** - каскадное объединение стилей (тег -> класс -> id -> состояние)
- **Async loading** - загрузка в фоновом потоке
- **Без зависимостей** - один `.hpp` файл, C++17, STL

---

## Подключение

Скопируйте `sess.hpp` в проект и включите:

```cpp
#include "sess.hpp"
using namespace fd::sess;
```

---

## Примеры

### Загрузка файла

```cpp
SESS sheet;
sheet.load("assets/ui/main.sess");

if (!sheet.ok()) {
    // файл не найден или не открылся //
}
```

### Прямой поиск по селектору

Возвращает стиль первого совпадения - без каскада.

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

// с fallback-значением //
float w = s.number("width", 100.f);
SESSColor c = s.color("color", {255, 255, 255});
```

### Загрузка из строки

Удобно для тестов или встроенных стилей:

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

### Resolve — каскадный поиск

`resolve` объединяет стили по CSS-приоритету: тег -> состояние тега -> классы -> id.  
Используйте его когда у элемента есть несколько классов или id.

```cpp
// sheet.resolve(тег, id, {классы}, состояние) //
auto s = sheet.resolve("button", "play", {".menu-button", ".large"}, SESSState::Hover);

widget.x      = s.number("x");
widget.y      = s.number("y");
widget.width  = s.number("width", 120.f);
widget.height = s.number("height", 36.f);
widget.bg     = s.color("background-color");
widget.anchor = s.anchor("anchor");
widget.pivot  = s.pivot("pivot");
```

`.sess` файл к этому примеру:

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

### Состояния

```cpp
// читаем состояние из переменной //
SESSState state = SESSState::None;
if (button.is_hovered()) state = SESSState::Hover;
if (button.is_pressed()) state = SESSState::Active;

auto s = sheet.resolve("button", "", {".menu-button"}, state);
widget.bg = s.color("background-color");
```

Состояния можно объединять через `|`:

```cpp
auto s = sheet["button:hover"];
// или через resolve с явным состоянием //
auto s = sheet.resolve("button", {}, {}, SESSState::Hover | SESSState::Focused);
```

### Асинхронная загрузка

```cpp
// запускаем загрузку в фоне //
auto handle = sess::load_async("assets/ui/main.sess");

// делаем другую работу //
init_renderer();
load_textures();

// ждём завершения //
handle.wait();

if (handle.data.ok()) {
    auto s = handle.data["button"];
    // ... //
}
```

Проверка без блокировки:

```cpp
if (handle.done()) {
    // загрузка завершена //
}
```

### Переменные и @include

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
sheet.load("ui/main.sess"); // подтянет все @include автоматически //
```

### Свободные функции

```cpp
// синхронная загрузка //
SESS sheet = sess::load("ui/main.sess");

// загрузка из строки //
SESS sheet = sess::from_string(R"(
    button { background: #333; }
)");

// асинхронная //
SESSResult handle = sess::load_async("ui/main.sess");
handle.wait();
```

---

## Типы значений в C++

### `SESSColor`

```cpp
SESSColor c = style.color("background-color");

c.r; c.g; c.b; c.a;             // uint8_t, 0–255 //
c.rf(); c.gf(); c.bf(); c.af(); // float, 0.0–1.0 //

c.to_rgba(); // uint32_t, RRGGBBAA //
c.to_argb(); // uint32_t, AARRGGBB (формат Windows GDI / Direct2D) хотя я бы его вообще не добавлял//
```

### `SESSValue`

Универсальный тип для любого свойства:

```cpp
const SESSValue& v = style.get("width");

v.is_number(); // float //
v.is_color();  // SESSColor //
v.is_string(); // std::string //
v.is_bool();   // bool //
v.is_none();   // свойство не задано //

v.number(0.f);         // float с fallback //
v.integer(0);          // int с fallback //
v.color({});           // SESSColor с fallback //
v.boolean(false);      // bool с fallback //
v.string_ref();        // const std::string& //
v.string_view();       // std::string_view //
```

### `UIAnchor` / `UIPivot`

```cpp
UIAnchor a = style.anchor("anchor", UIAnchor::TopLeft);
UIPivot  p = style.pivot("pivot",   UIPivot::Center);

// значения: TopLeft, Top, TopRight,          //
//           Left, Center, Right,             //
//           BottomLeft, Bottom, BottomRight  //
```

---

## Состояния (`SESSState`)

| Значение              | Синтаксис в `.sess` |
|-----------------------|---------------------|
| `SESSState::None`     | —                   |
| `SESSState::Hover`    | `:hover`            |
| `SESSState::Active`   | `:active`           |
| `SESSState::Focused`  | `:focus`            |
| `SESSState::Disabled` | `:disabled`         |
| `SESSState::Checked`  | `:checked`          |

---

## Требования

- **C++17** и выше
- Стандартная библиотека (STL)
- Никаких внешних зависимостей