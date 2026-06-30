#pragma once

///////////////////////////////////////////////////////////////////////////////////////
// sess.hpp - Single-header SESS style sheet parser (C++17)                          //
//                                                                                   //
// SESS is a simplified CSS-like format for game/app UI engines.                     //
//                                                                                   //
// syntax:                                                                           //
//   element { prop: value; }                                                        //
//   element:state { prop: value; }                                                  //
//   .class { prop: value; }                                                         //
//   #id { prop: value; }                                                            //
//   element.class:state { prop: value; }                                            //
//                                                                                   //
// usage:                                                                            //
//   SESS sheet;                                                                     //
//   sheet.load("assets/ui/main.sess");                                              //
//                                                                                   //
//   // Simple lookup //                                                             //
//   auto s = sheet["button"];                                                       //
//   button.set_color(s.color("background"));                                        //
//                                                                                   //
//   // full resolve (merges tag + id + classes + state, in CSS order) //            //  
//   auto s = sheet.resolve("button", "play", {".menu_button"}, SESSState::Hover);   //
//                                                                                   //
//   if (s.has("background"))                                                        //
//       button.set_color(s.color("background"));                                    //
//   if (s.has("font-size"))                                                         //
//       label.set_font_size(s.number("font-size"));                                 //
//                                                                                   //
// async:                                                                            //
//   auto handle = sess::load_async("ui.sess");                                      // 
//   handle.wait();                                                                  //
//   auto& sheet = handle.data;                                                      //
///////////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <variant>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <charconv>
#include <optional>
#include <atomic>
#include <future>

namespace fd::sess {

enum class UIAnchor {
    TopLeft,    Top,    TopRight,
    Left,       Center, Right,
    BottomLeft, Bottom, BottomRight
};

enum class UIPivot {
    TopLeft,    Top,    TopRight,
    Left,       Center, Right,
    BottomLeft, Bottom, BottomRight
};

enum class SESSState : uint32_t {
    None     = 0,
    Hover    = 1 << 0,
    Active   = 1 << 1,
    Focused  = 1 << 2,
    Disabled = 1 << 3,
    Checked  = 1 << 4,
};

inline SESSState operator|(SESSState a, SESSState b) {
    return static_cast<SESSState>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline bool operator&(SESSState a, SESSState b) {
    return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

struct SESSColor {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    SESSColor() = default;
    SESSColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    float rf() const { return r / 255.f; }
    float gf() const { return g / 255.f; }
    float bf() const { return b / 255.f; }
    float af() const { return a / 255.f; }

    uint32_t to_rgba() const {
        return (uint32_t(r) << 24) | (uint32_t(g) << 16) |
               (uint32_t(b) <<  8) | uint32_t(a);
    }

    uint32_t to_argb() const {
        return (uint32_t(a) << 24) | (uint32_t(r) << 16) |
               (uint32_t(g) <<  8) | uint32_t(b);
    }

    bool operator==(const SESSColor& o) const {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
};

struct SESSValue {
    enum class Type { Number, Color, String, Bool, None };

    using Storage = std::variant<
        std::monostate,  // none //
        float,           // number //
        SESSColor,       // color //
        std::string,     // string //
        bool             // bool //
    >;

    Storage data;

    SESSValue() : data(std::monostate{}) {}
    explicit SESSValue(float v)              : data(v)             {}
    explicit SESSValue(int v)                : data(float(v))      {}
    explicit SESSValue(SESSColor c)          : data(c)             {}
    explicit SESSValue(std::string s)        : data(std::move(s))  {}
    explicit SESSValue(std::string_view s)   : data(std::string(s)){}
    explicit SESSValue(bool b)               : data(b)             {}

    Type type() const {
        switch (data.index()) {
            case 0: return Type::None;
            case 1: return Type::Number;
            case 2: return Type::Color;
            case 3: return Type::String;
            case 4: return Type::Bool;
        }
        return Type::None;
    }

    bool is_none()   const { return data.index() == 0; }
    bool is_number() const { return data.index() == 1; }
    bool is_color()  const { return data.index() == 2; }
    bool is_string() const { return data.index() == 3; }
    bool is_bool()   const { return data.index() == 4; }

    float number(float fallback = 0.f) const {
        if (auto* p = std::get_if<float>(&data))  return *p;
        if (auto* p = std::get_if<bool>(&data))   return *p ? 1.f : 0.f;
        if (auto* p = std::get_if<std::string>(&data)) {
            float r = fallback;
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
            std::from_chars(p->data(), p->data() + p->size(), r);
#else
            try { r = std::stof(*p); } catch (...) {}
#endif
            return r;
        }
        return fallback;
    }

    int integer(int fallback = 0) const {
        return static_cast<int>(number(static_cast<float>(fallback)));
    }

    SESSColor color(SESSColor fallback = {}) const {
        if (auto* p = std::get_if<SESSColor>(&data)) return *p;
        return fallback;
    }

    bool boolean(bool fallback = false) const {
        if (auto* p = std::get_if<bool>(&data))   return *p;
        if (auto* p = std::get_if<float>(&data))  return *p != 0.f;
        if (auto* p = std::get_if<std::string>(&data)) {
            std::string_view sv(*p);
            if (sv == "true" || sv == "yes" || sv == "1") return true;
            if (sv == "false"|| sv == "no"  || sv == "0") return false;
        }
        return fallback;
    }

    const std::string& string_ref() const {
        static const std::string empty;
        if (auto* p = std::get_if<std::string>(&data)) return *p;
        return empty;
    }

    std::string_view string_view() const { return string_ref(); }
};

struct SESSStyle {
    std::unordered_map<std::string, SESSValue> props;

    bool has(std::string_view key) const {
        return props.count(std::string(key)) > 0;
    }

    const SESSValue& get(std::string_view key) const {
        static const SESSValue empty;
        auto it = props.find(std::string(key));
        return it != props.end() ? it->second : empty;
    }

    float number(std::string_view key, float fallback = 0.f) const {
        return get(key).number(fallback);
    }

    int integer(std::string_view key, int fallback = 0) const {
        return get(key).integer(fallback);
    }

    SESSColor color(std::string_view key, SESSColor fallback = {}) const {
        return get(key).color(fallback);
    }

    bool boolean(std::string_view key, bool fallback = false) const {
        return get(key).boolean(fallback);
    }

    const std::string& string(std::string_view key) const {
        return get(key).string_ref();
    }

    UIAnchor anchor(std::string_view key, UIAnchor fallback = UIAnchor::TopLeft) const {
        auto it = props.find(std::string(key));
        if (it == props.end()) return fallback;
        std::string_view sv = it->second.string_view();
        if (sv == "top-left"    || sv == "topleft")    return UIAnchor::TopLeft;
        if (sv == "top"         || sv == "top-center") return UIAnchor::Top;
        if (sv == "top-right"   || sv == "topright")   return UIAnchor::TopRight;
        if (sv == "left"        || sv == "mid-left")   return UIAnchor::Left;
        if (sv == "center"      || sv == "middle")     return UIAnchor::Center;
        if (sv == "right"       || sv == "mid-right")  return UIAnchor::Right;
        if (sv == "bottom-left" || sv == "bottomleft") return UIAnchor::BottomLeft;
        if (sv == "bottom"      || sv == "bottom-center") return UIAnchor::Bottom;
        if (sv == "bottom-right"|| sv == "bottomright")return UIAnchor::BottomRight;
        return fallback;
    }

    UIPivot pivot(std::string_view key, UIPivot fallback = UIPivot::TopLeft) const {
        UIAnchor a = anchor(key, static_cast<UIAnchor>(static_cast<int>(fallback)));
        return static_cast<UIPivot>(static_cast<int>(a));
    }

    SESSStyle& merge(const SESSStyle& other) {
        for (auto& [k, v] : other.props)
            props[k] = v;
        return *this;
    }

    const SESSValue& operator[](std::string_view key) const { return get(key); }
};

namespace detail {

inline std::string_view trim(std::string_view s) {
    constexpr std::string_view ws = " \t\r\n";
    auto b = s.find_first_not_of(ws);
    if (b == std::string_view::npos) return {};
    auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

inline std::string trim_str(std::string_view s) {
    return std::string(trim(s));
}

// #RGB / #RRGGBB / #RGBA / #RRGGBBAA //
inline std::optional<SESSColor> parse_hex_color(std::string_view s) {
    if (s.empty() || s[0] != '#') return std::nullopt;
    s.remove_prefix(1);

    auto hex1 = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return uint8_t(c - '0');
        if (c >= 'a' && c <= 'f') return uint8_t(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return uint8_t(c - 'A' + 10);
        return 0;
    };

    if (s.size() == 3) {
        // #RGB → #RRGGBB //
        uint8_t r = hex1(s[0]); r = (r << 4) | r;
        uint8_t g = hex1(s[1]); g = (g << 4) | g;
        uint8_t b = hex1(s[2]); b = (b << 4) | b;
        return SESSColor{ r, g, b, 255 };
    }
    if (s.size() == 4) {
        // #RGBA //
        uint8_t r = hex1(s[0]); r = (r << 4) | r;
        uint8_t g = hex1(s[1]); g = (g << 4) | g;
        uint8_t b = hex1(s[2]); b = (b << 4) | b;
        uint8_t a = hex1(s[3]); a = (a << 4) | a;
        return SESSColor{ r, g, b, a };
    }
    if (s.size() == 6) {
        uint8_t r = (hex1(s[0]) << 4) | hex1(s[1]);
        uint8_t g = (hex1(s[2]) << 4) | hex1(s[3]);
        uint8_t b = (hex1(s[4]) << 4) | hex1(s[5]);
        return SESSColor{ r, g, b, 255 };
    }
    if (s.size() == 8) {
        uint8_t r = (hex1(s[0]) << 4) | hex1(s[1]);
        uint8_t g = (hex1(s[2]) << 4) | hex1(s[3]);
        uint8_t b = (hex1(s[4]) << 4) | hex1(s[5]);
        uint8_t a = (hex1(s[6]) << 4) | hex1(s[7]);
        return SESSColor{ r, g, b, a };
    }
    return std::nullopt;
}

inline std::optional<SESSColor> parse_named_color(std::string_view s) {
    struct Named { const char* name; uint8_t r,g,b; };
    static const Named table[] = {
        {"black",   0,   0,   0  },
        {"white",   255, 255, 255},
        {"red",     255, 0,   0  },
        {"green",   0,   128, 0  },
        {"blue",    0,   0,   255},
        {"yellow",  255, 255, 0  },
        {"cyan",    0,   255, 255},
        {"magenta", 255, 0,   255},
        {"orange",  255, 165, 0  },
        {"purple",  128, 0,   128},
        {"pink",    255, 192, 203},
        {"gray",    128, 128, 128},
        {"grey",    128, 128, 128},
        {"silver",  192, 192, 192},
        {"transparent", 0, 0, 0  },
    };
    for (auto& n : table) {
        if (s == n.name) {
            uint8_t a = (s == "transparent") ? 0 : 255;
            return SESSColor{ n.r, n.g, n.b, a };
        }
    }
    return std::nullopt;
}

inline std::optional<SESSColor> parse_rgb_func(std::string_view s) {
    bool has_alpha = s.size() > 5 && s.substr(0, 5) == "rgba(";
    bool no_alpha  = s.size() > 4 && s.substr(0, 4) == "rgb(";
    if (!has_alpha && !no_alpha) return std::nullopt;

    size_t close = s.rfind(')');
    if (close == std::string_view::npos) return std::nullopt;

    std::string_view inner = s.substr(has_alpha ? 5 : 4, close - (has_alpha ? 5 : 4));

    std::vector<std::string_view> parts;
    size_t pos = 0;
    while (pos <= inner.size()) {
        size_t comma = inner.find(',', pos);
        if (comma == std::string_view::npos) comma = inner.size();
        parts.push_back(trim(inner.substr(pos, comma - pos)));
        pos = comma + 1;
    }

    auto parse_byte = [](std::string_view sv, uint8_t& out) -> bool {
        sv = trim(sv);
        int v = 0;
        auto [p, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), v);
        if (ec != std::errc{}) return false;
        out = static_cast<uint8_t>(std::clamp(v, 0, 255));
        return true;
    };

    SESSColor c;
    if (parts.size() >= 3) {
        if (!parse_byte(parts[0], c.r)) return std::nullopt;
        if (!parse_byte(parts[1], c.g)) return std::nullopt;
        if (!parse_byte(parts[2], c.b)) return std::nullopt;
        if (parts.size() >= 4) {
            std::string_view av = trim(parts[3]);
            float fa = 1.f;
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
            std::from_chars(av.data(), av.data() + av.size(), fa);
#else
            try { fa = std::stof(std::string(av)); } catch(...) {}
#endif
            c.a = (fa <= 1.f) ? uint8_t(fa * 255.f)
                               : static_cast<uint8_t>(std::clamp(int(fa), 0, 255));
        }
        return c;
    }
    return std::nullopt;
}

inline std::optional<SESSColor> parse_color(std::string_view s) {
    s = trim(s);
    if (!s.empty() && s[0] == '#')      return parse_hex_color(s);
    if (s.size() > 3 && s[0] == 'r' &&
        s[1] == 'g' && s[2] == 'b')     return parse_rgb_func(s);
    return parse_named_color(s);
}

// substitute @varname in a raw value string using the vars map //
inline std::string substitute_vars(std::string_view raw,
    const std::unordered_map<std::string, std::string>& vars)
{
    if (raw.find('@') == std::string_view::npos) return std::string(raw);
    std::string result;
    result.reserve(raw.size() + 32);
    size_t pos = 0;
    while (pos < raw.size()) {
        size_t at = raw.find('@', pos);
        if (at == std::string_view::npos) { result.append(raw.substr(pos)); break; }
        result.append(raw.substr(pos, at - pos));
        // collect identifier chars after '@' //
        size_t end = at + 1;
        while (end < raw.size() && (std::isalnum((unsigned char)raw[end]) || raw[end] == '_'))
            ++end;
        if (end > at + 1) {
            std::string var_name(raw.substr(at + 1, end - at - 1));
            auto it = vars.find(var_name);
            if (it != vars.end())
                result += it->second;
            else {
                result += '@';
                result += var_name;
            }
            pos = end;
        } else {
            result += '@';
            pos = at + 1;
        }
    }
    return result;
}

inline SESSValue parse_value(std::string_view raw) {
    raw = trim(raw);
    if (raw.empty()) return SESSValue{};

    if (raw == "true"  || raw == "yes") return SESSValue(true);
    if (raw == "false" || raw == "no")  return SESSValue(false);

    if (auto c = parse_color(raw)) return SESSValue(*c);

    {
        std::string_view num = raw;
        if (num.size() > 2 && num.substr(num.size()-2) == "px") num.remove_suffix(2);
        else if (num.size() > 2 && num.substr(num.size()-2) == "em") num.remove_suffix(2);
        else if (!num.empty() && num.back() == '%') num.remove_suffix(1);

        int ival = 0;
        auto [p1, ec1] = std::from_chars(num.data(), num.data() + num.size(), ival);
        if (ec1 == std::errc{} && p1 == num.data() + num.size())
            return SESSValue(float(ival));

        float fval = 0.f;
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
        auto [p2, ec2] = std::from_chars(num.data(), num.data() + num.size(), fval);
        if (ec2 == std::errc{} && p2 == num.data() + num.size())
            return SESSValue(fval);
#else
        try {
            size_t idx = 0;
            fval = std::stof(std::string(num), &idx);
            if (idx == num.size()) return SESSValue(fval);
        } catch (...) {}
#endif
    }

    if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"')
        raw = raw.substr(1, raw.size() - 2);

    return SESSValue(std::string(raw));
}

struct Selector {
    std::string  tag;      
    std::string  id;       
    std::vector<std::string> classes; 
    SESSState    state = SESSState::None;

    std::string key() const {
        std::string k;
        k += tag;
        auto sorted = classes;
        std::sort(sorted.begin(), sorted.end());
        for (auto& c : sorted) { k += '.'; k += c; }
        if (!id.empty()) { k += '#'; k += id; }
        if (state != SESSState::None) {
            k += ':';
            k += state_str(state);
        }
        return k;
    }

    static std::string state_str(SESSState s) {
        switch (s) {
            case SESSState::Hover:    return "hover";
            case SESSState::Active:   return "active";
            case SESSState::Focused:  return "focus";
            case SESSState::Disabled: return "disabled";
            case SESSState::Checked:  return "checked";
            default:                  return "";
        }
    }
};

inline Selector parse_selector(std::string_view raw) {
    raw = trim(raw);
    Selector sel;

    size_t colon = raw.rfind(':');
    if (colon != std::string_view::npos) {
        std::string_view state_sv = raw.substr(colon + 1);
        SESSState st = SESSState::None;
        if (state_sv == "hover")    st = SESSState::Hover;
        else if (state_sv == "active")  st = SESSState::Active;
        else if (state_sv == "focus" || state_sv == "focused") st = SESSState::Focused;
        else if (state_sv == "disabled") st = SESSState::Disabled;
        else if (state_sv == "checked")  st = SESSState::Checked;

        if (st != SESSState::None) {
            sel.state = st;
            raw = raw.substr(0, colon);
        }
    }

    std::string token;
    char        prefix = 0; 

    auto flush = [&]() {
        if (token.empty()) return;
        if (prefix == '.') sel.classes.push_back(token);
        else if (prefix == '#') sel.id = token;
        else sel.tag = token;
        token.clear();
    };

    for (char c : raw) {
        if (c == '.' || c == '#') {
            flush();
            prefix = c;
        } else {
            token += c;
        }
    }
    flush();

    return sel;
}

struct Rule {
    Selector   selector;
    SESSStyle  style;
    int        source_order = 0; 
};

}

class SESS {
public:
    SESS() = default;

    bool load(std::string_view path) {
        rules_.clear();
        vars_.clear();
        source_path_ = std::string(path);

        FILE* f = fopen(source_path_.c_str(), "r");
        if (!f) {
            std::cerr << "[SESS] Failed to open: " << path << "\n";
            ok_ = false;
            return false;
        }

        std::string src;
        char buf[4096];
        while (fgets(buf, sizeof(buf), f))
            src += buf;
        fclose(f);

        // compute base directory for @include resolution //
        std::string base_dir;
        size_t sl = source_path_.find_last_of("/\\");
        if (sl != std::string::npos) base_dir = source_path_.substr(0, sl + 1);

        parse_source_(src, vars_, base_dir);
        ok_ = true;
        return true;
    }

    bool load_from_string(std::string_view source, std::string_view base_dir = {}) {
        rules_.clear();
        vars_.clear();
        parse_source_(std::string(source), vars_, base_dir);
        ok_ = true;
        return true;
    }

    bool ok()  const { return ok_; }

    SESSStyle operator[](std::string_view selector_str) const {
        auto sel = detail::parse_selector(selector_str);
        for (auto it = rules_.rbegin(); it != rules_.rend(); ++it) {
            if (it->selector.key() == sel.key())
                return it->style;
        }
        return {};
    }

    SESSStyle resolve(
        std::string_view          tag,
        std::string_view          id       = {},
        std::vector<std::string>  classes  = {},
        SESSState                 state    = SESSState::None
    ) const {
        SESSStyle result;

        result.merge(find_style_(tag, {}, {}, SESSState::None));

        if (state != SESSState::None)
            result.merge(find_style_(tag, {}, {}, state));
        for (auto& cls : classes) {
            result.merge(find_style_({}, {}, cls, SESSState::None));
            if (state != SESSState::None)
                result.merge(find_style_({}, {}, cls, state));
        }
        if (!id.empty()) {
            result.merge(find_style_({}, id, {}, SESSState::None));
            if (state != SESSState::None)
                result.merge(find_style_({}, id, {}, state));
        }

        return result;
    }

    SESSStyle resolve(
        std::string_view                  tag,
        std::string_view                  id,
        std::initializer_list<const char*> classes,
        SESSState                          state = SESSState::None
    ) const {
        std::vector<std::string> cv;
        cv.reserve(classes.size());
        for (auto* c : classes) {
            std::string_view sv(c);
            if (!sv.empty() && sv[0] == '.') sv.remove_prefix(1);
            cv.emplace_back(sv);
        }
        return resolve(tag, id, std::move(cv), state);
    }

    const std::vector<detail::Rule>& rules() const { return rules_; }

private:

    SESSStyle find_style_(
        std::string_view              tag,
        std::string_view              id,
        std::string_view              single_class,
        SESSState                     state
    ) const {
        detail::Selector probe;
        probe.tag   = std::string(tag);
        probe.id    = std::string(id);
        if (!single_class.empty()) {
            std::string cls(single_class);
            if (!cls.empty() && cls[0] == '.') cls.erase(0, 1);
            probe.classes.push_back(cls);
        }
        probe.state = state;
        std::string want = probe.key();

        SESSStyle result;
        for (auto& rule : rules_) {
            if (rule.selector.key() == want)
                result.merge(rule.style); 
        }
        return result;
    }

    void parse_source_(const std::string& src,
                       std::unordered_map<std::string, std::string>& vars,
                       std::string_view base_dir,
                       int depth = 0)
    {
        std::string preprocessed = preprocess_(src, vars, base_dir, depth);
        std::string cleaned = strip_comments_(preprocessed);
        parse_blocks_(cleaned, vars);
    }

    // preprocess: handle @set and @include directives before CSS-block parsing //
    std::string preprocess_(const std::string& src,
                            std::unordered_map<std::string, std::string>& vars,
                            std::string_view base_dir,
                            int depth)
    {
        if (depth > 8) {
            std::cerr << "[SESS] @include depth limit reached\n";
            return {};
        }
        std::string out;
        out.reserve(src.size());
        size_t pos = 0;
        while (pos < src.size()) {
            // find next newline to get a line //
            size_t nl = src.find('\n', pos);
            if (nl == std::string_view::npos) nl = src.size();
            std::string_view line(src.data() + pos, nl - pos);
            pos = nl + 1;

            // trim for directive detection //
            std::string_view sv = detail::trim(line);

            // @set VAR = value //
            if (sv.size() >= 4 && sv.substr(0, 4) == "@set" &&
                (sv.size() == 4 || sv[4] == ' ' || sv[4] == '\t'))
            {
                auto rest = detail::trim(sv.substr(4));
                size_t eq = rest.find('=');
                if (eq != std::string_view::npos) {
                    auto vn = detail::trim_str(rest.substr(0, eq));
                    auto vv = detail::trim_str(rest.substr(eq + 1));
                    
                    if (!vv.empty() && vv.back() == ';') vv.pop_back();
                    vv = detail::trim_str(vv);
                    
                    vars[vn] = detail::substitute_vars(vv, vars);
                }
            
                out += '\n';
                continue;
            }

            // @include "path" //
            if (sv.size() >= 8 && sv.substr(0, 8) == "@include" &&
                (sv.size() == 8 || sv[8] == ' ' || sv[8] == '\t'))
            {
                auto inc = detail::trim(sv.substr(8));
                // strip surrounding quotes //
                if (inc.size() >= 2 && inc.front() == '"' && inc.back() == '"')
                    inc = inc.substr(1, inc.size() - 2);
                std::string inc_path;
                inc_path.reserve(base_dir.size() + inc.size());
                inc_path.append(base_dir);
                inc_path.append(inc);

                FILE* f = fopen(inc_path.c_str(), "r");
                if (!f) {
                    std::cerr << "[SESS] @include: failed to open: " << inc_path << "\n";
                } else {
                    std::string inc_src;
                    char buf[4096];
                    while (fgets(buf, sizeof(buf), f)) inc_src += buf;
                    fclose(f);
                    // compute sub-dir for nested includes //
                    std::string sub_dir;
                    size_t sl = inc_path.find_last_of("/\\");
                    if (sl != std::string::npos) sub_dir = inc_path.substr(0, sl + 1);
                    out += preprocess_(inc_src, vars, sub_dir, depth + 1);
                }
                out += '\n';
                continue;
            }

            // regular line - emit as-is //
            out.append(line);
            out += '\n';
        }
        return out;
    }

    std::string strip_comments_(const std::string& src) {
        std::string out;
        out.reserve(src.size());
        size_t i = 0;
        while (i < src.size()) {
            if (i + 1 < src.size() && src[i] == '/' && src[i+1] == '*') {
                i += 2;
                while (i + 1 < src.size()) {
                    if (src[i] == '*' && src[i+1] == '/') { i += 2; break; }
                    ++i;
                }
                out += ' ';
                continue;
            }
            if (i + 1 < src.size() && src[i] == '/' && src[i+1] == '/') {
                while (i < src.size() && src[i] != '\n') ++i;
                continue;
            }
            out += src[i++];
        }
        return out;
    }

    void parse_blocks_(const std::string& src,
                       const std::unordered_map<std::string, std::string>& vars) {
        int order = 0;
        size_t i = 0;
        while (i < src.size()) {
            while (i < src.size() && std::isspace((unsigned char)src[i])) ++i;
            if (i >= src.size()) break;

            size_t sel_start = i;
            while (i < src.size() && src[i] != '{') ++i;
            if (i >= src.size()) break;

            std::string sel_raw(src.data() + sel_start, i - sel_start);
            ++i;

            size_t body_start = i;
            int depth = 1;
            while (i < src.size() && depth > 0) {
                if (src[i] == '{') ++depth;
                if (src[i] == '}') --depth;
                ++i;
            }
            std::string body(src.data() + body_start, i - body_start - 1);

            std::vector<std::string_view> selectors = split_selectors_(sel_raw);
            SESSStyle block_style = parse_decl_block_(body, vars);

            for (auto sv : selectors) {
                auto s = detail::trim(sv);
                if (s.empty()) continue;
                detail::Rule rule;
                rule.selector    = detail::parse_selector(s);
                rule.style       = block_style;
                rule.source_order = order++;
                rules_.push_back(std::move(rule));
            }
        }
    }

    std::vector<std::string_view> split_selectors_(std::string_view s) {
        std::vector<std::string_view> result;
        size_t pos = 0;
        while (pos < s.size()) {
            size_t comma = s.find(',', pos);
            if (comma == std::string_view::npos) comma = s.size();
            result.push_back(s.substr(pos, comma - pos));
            pos = comma + 1;
        }
        return result;
    }

    SESSStyle parse_decl_block_(const std::string& body,
                                const std::unordered_map<std::string, std::string>& vars) {
        SESSStyle style;
        size_t pos = 0;
        while (pos < body.size()) {
            size_t semi = body.find(';', pos);
            if (semi == std::string_view::npos) semi = body.size();
            std::string_view decl(body.data() + pos, semi - pos);
            decl = detail::trim(decl);
            if (!decl.empty()) {
                size_t colon = decl.find(':');
                if (colon != std::string_view::npos) {
                    auto key = detail::trim_str(decl.substr(0, colon));
                    auto raw_val = detail::trim_str(decl.substr(colon + 1));
                    if (!key.empty()) {
                        // substitute @variable references in value //
                        std::string subst = detail::substitute_vars(raw_val, vars);
                        style.props[key] = detail::parse_value(subst);
                    }
                }
            }
            pos = semi + 1;
        }
        return style;
    }

    std::vector<detail::Rule> rules_;
    std::string source_path_;
    std::unordered_map<std::string, std::string> vars_;
    bool ok_ = false;
};

struct SESSResult {
    SESS data;
    bool ok = false;
    std::shared_future<void> ready;

    void wait() const {
        if (ready.valid()) ready.wait();
    }

    bool done() const {
        if (!ready.valid()) return true;
        return ready.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
};

namespace sess {

inline SESS load(std::string_view path) {
    SESS sheet;
    sheet.load(path);
    return sheet;
}

inline SESS from_string(std::string_view source) {
    SESS sheet;
    sheet.load_from_string(source);
    return sheet;
}

inline SESSResult load_async(std::string_view path) {
    SESSResult result;
    std::string p(path);
    auto promise = std::make_shared<std::promise<void>>();
    result.ready = promise->get_future().share();

    std::thread([p, &result, promise = std::move(promise)]() mutable {
        result.ok = result.data.load(p);
        promise->set_value();
    }).detach();

    return result;
}

} 

}