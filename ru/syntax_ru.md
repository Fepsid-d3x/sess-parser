# Справочник синтаксиса SESS

SESS - упрощённый CSS-подобный формат для описания стилей UI в играх и приложениях.  
Файлы имеют расширение `.sess`.

---

## Комментарии

```css
// однострочный комментарий 

/* многострочный
   комментарий */
```

---

## Переменные (`@set`)

Объявляются в любом месте файла до использования.  
Значение может содержать цвет, число или строку.  
Точка с запятой в конце - опциональна.

```css
@set surface    = #89b4faff;
@set on-surface = #cdd6f4ff;
@set radius     = 6;
@set font-main  = Roboto;
```

Использование переменной - `@имя`:

```css
button {
    background-color: @surface;
    color: @on-surface;
    border-radius: @radius;
    font-family: @font-main;
}
```

Переменные можно строить друг из друга:

```css
@set base-pad  = 8;
@set inner-pad = @base-pad;
```

---

## Включение файлов (`@include`)

Подключает другой `.sess` файл. Путь - относительно текущего файла.  
Вложенность — до 8 уровней.

```css
@include "theme/colors.sess"
@include "theme/typography.sess"
@include "components/buttons.sess"
```

Типичная структура проекта:

```
ui/
├── main.sess           ← точка входа
├── theme/
│   ├── colors.sess     ← @set переменные цветов
│   └── typography.sess ← @set шрифты, размеры
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

## Правила (Rules)

Общий синтаксис:

```
селектор {
    свойство: значение;
    свойство: значение;
}
```

---

## Селекторы

### По тегу

```css
button {
    background-color: #303030;
}

label {
    font-size: 14;
}
```

### По классу (`.`)

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

### По ID (`#`)

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

### Комбинированный

Тег, классы и ID можно комбинировать в любом порядке:

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

### Группировка (через запятую)

```css
button, .clickable, #submit {
    cursor: pointer;
}
```

---

## Состояния (States)

Состояние добавляется через `:` после селектора.

| Состояние   | Синтаксис   |
|-------------|-------------|
| Наведение   | `:hover`    |
| Нажатие     | `:active`   |
| Фокус       | `:focus`    |
| Отключён    | `:disabled` |
| Включён     | `:checked`  |

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

Состояния работают со всеми видами селекторов:

```css
button.menu-button:hover {
    background-color: @surface-hover;
}

#play-button:active {
    scale: 0.95;
}
```

---

## Типы значений

### Числа

Числа без единиц, с `px`, `em`, или `%` - всё читается как `float`.

```css
element {
    width: 128;
    height: 32px;
    opacity: 0.75;
    font-size: 16em;
    margin: 50%;
}
```

### Цвета

Поддерживаемые форматы:

```css
element {
    /* HEX */
    color: #fff;             /* #RGB  -> непрозрачный */
    color: #fffc;            /* #RGBA */
    color: #ffffff;          /* #RRGGBB */
    color: #ffffffff;        /* #RRGGBBAA */

    /* RGB / RGBA функции */
    color: rgb(255, 128, 0);
    color: rgba(255, 128, 0, 0.5);

    /* Именованные */
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

### Булевы значения

```css
element {
    visible: true;
    enabled: false;
    clip: yes;
    shadow: no;
}
```

### Строки

Строки без кавычек или в кавычках:

```css
element {
    font-family: Roboto;
    text: "Hello, World!";
    align: center;
}
```

### Anchor и Pivot

Специальные строковые значения для позиционирования:

```css
element {
    anchor: top-left;      /* или topleft */
    anchor: top;
    anchor: top-right;
    anchor: left;
    anchor: center;        /* или middle */
    anchor: right;
    anchor: bottom-left;
    anchor: bottom;
    anchor: bottom-right;

    pivot: center;
}
```

---

## Полный пример файла

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