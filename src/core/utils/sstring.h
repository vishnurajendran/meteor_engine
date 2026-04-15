#pragma once

#include <algorithm>
#include <charconv>
#include <functional>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#define STR(...) SString(__VA_ARGS__)

struct SString
{
private:
    std::string coreStr;

    // ── Internal formatting helpers ───────────────────────────────────────────

    // Converts a single argument to std::string, respecting an optional
    // format specifier string (the part after ':' in {N:spec}).
    //
    // Supported specifiers:
    //   fN   — fixed-point float with N decimal places   e.g. {0:f2}
    //   x    — lowercase hexadecimal integer             e.g. {0:x}
    //   X    — uppercase hexadecimal integer             e.g. {0:X}
    //   eN   — scientific notation with N decimal places e.g. {0:e3}
    //   0N   — zero-padded integer to width N            e.g. {0:04}
    //   +    — show sign for positive numbers            e.g. {0:+}
    //   (empty) — default std::to_string / SString::str()

    template<typename T>
    static std::string applySpec(T&& val, const std::string& spec)
    {
        using D = std::decay_t<T>;

        // SString passthrough
        if constexpr (std::is_same_v<D, SString>)
            return val.coreStr;

        // std::string passthrough
        else if constexpr (std::is_same_v<D, std::string>)
            return val;

        // const char* passthrough
        else if constexpr (std::is_convertible_v<D, const char*>)
            return std::string(val);

        // bool
        else if constexpr (std::is_same_v<D, bool>)
            return val ? "true" : "false";

        // Numeric types — inspect specifier
        else if constexpr (std::is_arithmetic_v<D>)
        {
            if (spec.empty())
                return std::to_string(val);

            std::ostringstream oss;

            // f  → fixed float, optional digit count: "f2", "f4" …
            if (spec[0] == 'f')
            {
                int prec = (spec.size() > 1) ? std::stoi(spec.substr(1)) : 6;
                oss << std::fixed << std::setprecision(prec) << val;
                return oss.str();
            }

            // e  → scientific notation: "e3" …
            if (spec[0] == 'e' || spec[0] == 'E')
            {
                int prec = (spec.size() > 1) ? std::stoi(spec.substr(1)) : 6;
                oss << (spec[0] == 'E' ? std::uppercase : std::nouppercase)
                    << std::scientific << std::setprecision(prec) << val;
                return oss.str();
            }

            // x / X  → hexadecimal integer
            if (spec[0] == 'x' || spec[0] == 'X')
            {
                if constexpr (std::is_integral_v<D>)
                {
                    oss << (spec[0] == 'X' ? std::uppercase : std::nouppercase)
                        << std::hex << val;
                    return oss.str();
                }
                return std::to_string(val);
            }

            // 0N  → zero-padded integer width: "04", "08" …
            if (spec[0] == '0' && spec.size() > 1)
            {
                int width = std::stoi(spec.substr(1));
                oss << std::setfill('0') << std::setw(width) << val;
                return oss.str();
            }

            // +  → force sign
            if (spec[0] == '+')
            {
                oss << std::showpos << val;
                return oss.str();
            }

            return std::to_string(val);
        }

        // Fallback: try streaming into ostringstream
        else
        {
            std::ostringstream oss;
            oss << val;
            return oss.str();
        }
    }

    // Replaces all occurrences of `token` inside `str` with `replacement`.
    static void replaceAll(std::string& str,
                           const std::string& token,
                           const std::string& replacement)
    {
        size_t pos = 0;
        while ((pos = str.find(token, pos)) != std::string::npos)
        {
            str.replace(pos, token.size(), replacement);
            pos += replacement.size();
        }
    }

public:
    // ── Constructors / assignment ─────────────────────────────────────────────

    SString()                            : coreStr() {}
    SString(const char* s)               : coreStr(s ? s : "") {}
    SString(const std::string& s)        : coreStr(s) {}
    SString(const SString& o)            : coreStr(o.coreStr) {}
    SString(SString&& o) noexcept        : coreStr(std::move(o.coreStr)) {}
    SString(char c, size_t count = 1)    : coreStr(count, c) {}  // NEW: fill with char

    SString& operator=(const SString& o)   { if (this != &o) coreStr = o.coreStr; return *this; }
    SString& operator=(SString&& o) noexcept { coreStr = std::move(o.coreStr); return *this; }
    SString& operator=(const char* s)       { coreStr = s ? s : ""; return *this; }
    SString& operator=(const std::string& s){ coreStr = s; return *this; }

    // ── Conversion ────────────────────────────────────────────────────────────

    operator std::string() const { return coreStr; }
    const char* c_str()    const { return coreStr.c_str(); }
    std::string str()      const { return coreStr; }

    // ── Static factory ────────────────────────────────────────────────────────

    static SString fromInt(int v)             { return std::to_string(v); }
    static SString fromUInt(unsigned int v)   { return std::to_string(v); }
    static SString fromLong(long long v)      { return std::to_string(v); }
    static SString fromFloat(float v, int precision = 6)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision) << v;
        return ss.str();
    }
    static SString fromDouble(double v, int precision = 10)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision) << v;
        return ss.str();
    }
    static SString fromBool(bool v)  { return v ? "true" : "false"; }
    static SString fromChar(char c)  { return SString(c); }

    // ── Parsing ───────────────────────────────────────────────────────────────

    [[nodiscard]] int    toInt()    const { return std::stoi(coreStr); }
    [[nodiscard]] long   toLong()   const { return std::stol(coreStr); }
    [[nodiscard]] float  toFloat()  const { return std::stof(coreStr); }
    [[nodiscard]] double toDouble() const { return std::stod(coreStr); }
    [[nodiscard]] bool   toBool()   const { return coreStr == "true" || coreStr == "1"; }

    // Safe variants — return false and leave `out` unchanged on failure.
    bool tryParseInt(int& out)       const noexcept { try { out = std::stoi(coreStr); return true; } catch(...){ return false; } }
    bool tryParseFloat(float& out)   const noexcept { try { out = std::stof(coreStr); return true; } catch(...){ return false; } }
    bool tryParseDouble(double& out) const noexcept { try { out = std::stod(coreStr); return true; } catch(...){ return false; } }

    // ── C#-style formatting ───────────────────────────────────────────────────
    //
    // Usage:
    //   SString::format("Hello {0}, you scored {1:f2} points!", "Alice", 98.765f)
    //   → "Hello Alice, you scored 98.77 points!"
    //
    //   SString::format("0x{0:X} ({0:04})", 255)
    //   → "0xFF (0255)"       (same index can appear multiple times)
    //
    // Specifiers after ':' (optional):
    //   fN  — fixed float, N decimal places   {0:f2}
    //   eN  — scientific notation N decimals  {0:e3}
    //   x   — lowercase hex                   {0:x}
    //   X   — uppercase hex                   {0:X}
    //   0N  — zero-padded width N             {0:04}
    //   +   — force sign                      {0:+}

    template<typename... Args>
    static SString format(const SString& fmt, Args&&... args)
    {
        // Store one converter lambda per argument.
        // Each lambda closes over a copy of its argument and applies the
        // format specifier on demand, so the same index can appear multiple
        // times in the format string with different specifiers.
        //
        // We use the std::initializer_list expansion trick instead of a fold
        // expression because GCC cannot always see the pack inside a lambda
        // capture when writing  (push_back([v = fwd(args)](...){...}), ...).
        std::vector<std::function<std::string(const std::string&)>> converters;
        converters.reserve(sizeof...(Args));
        (void)std::initializer_list<int>{
            (converters.push_back(
                [v = std::forward<Args>(args)](const std::string& spec) mutable -> std::string {
                    return applySpec(v, spec);
                }
            ), 0)...
        };

        std::string result = fmt.coreStr;
        std::string output;
        output.reserve(result.size());

        size_t i = 0;
        while (i < result.size())
        {
            // Escaped brace: {{ → {, }} → }
            if (result[i] == '{' && i + 1 < result.size() && result[i+1] == '{')
            { output += '{'; i += 2; continue; }
            if (result[i] == '}' && i + 1 < result.size() && result[i+1] == '}')
            { output += '}'; i += 2; continue; }

            // Format token: {N} or {N:spec}
            if (result[i] == '{')
            {
                size_t close = result.find('}', i + 1);
                if (close == std::string::npos)
                    throw std::invalid_argument("SString::format — unclosed '{' in format string");

                std::string token   = result.substr(i + 1, close - i - 1);
                std::string spec;
                size_t colon = token.find(':');
                if (colon != std::string::npos)
                {
                    spec  = token.substr(colon + 1);
                    token = token.substr(0, colon);
                }

                size_t idx = static_cast<size_t>(std::stoul(token));
                if (idx >= converters.size())
                    throw std::out_of_range("SString::format — argument index out of range");

                output += converters[idx](spec);
                i = close + 1;
                continue;
            }

            output += result[i++];
        }

        return SString(output);
    }

    // Convenience: single-argument shorthand.
    // e.g. SString("Value: {0}").fmt(42)
    template<typename... Args>
    SString fmt(Args&&... args) const
    {
        return format(*this, std::forward<Args>(args)...);
    }

    // ── Arithmetic operators ──────────────────────────────────────────────────

    SString  operator+ (const SString& o)     const { return coreStr + o.coreStr; }
    SString  operator+ (const char* s)         const { return coreStr + s; }
    SString  operator+ (const std::string& s)  const { return coreStr + s; }
    SString  operator+ (char c)                const { return coreStr + c; }

    SString& operator+=(const SString& o)            { coreStr += o.coreStr; return *this; }
    SString& operator+=(const char* s)               { coreStr += s; return *this; }
    SString& operator+=(const std::string& s)        { coreStr += s; return *this; }
    SString& operator+=(char c)                      { coreStr += c; return *this; }

    // Free-function left-hand concat (char*, std::string, char on the left)
    friend SString operator+(const char* lhs,        const SString& rhs) { return std::string(lhs) + rhs.coreStr; }
    friend SString operator+(const std::string& lhs, const SString& rhs) { return lhs + rhs.coreStr; }
    friend SString operator+(char lhs,               const SString& rhs) { return std::string(1, lhs) + rhs.coreStr; }

    // ── Comparison operators ──────────────────────────────────────────────────

    bool operator==(const SString& o)    const { return coreStr == o.coreStr; }
    bool operator!=(const SString& o)    const { return coreStr != o.coreStr; }
    bool operator< (const SString& o)    const { return coreStr <  o.coreStr; }
    bool operator> (const SString& o)    const { return coreStr >  o.coreStr; }
    bool operator<=(const SString& o)    const { return coreStr <= o.coreStr; }  // NEW
    bool operator>=(const SString& o)    const { return coreStr >= o.coreStr; }  // NEW

    // ── Element access ────────────────────────────────────────────────────────

    char& operator[](size_t i)       { return coreStr[i]; }
    const char& operator[](size_t i) const { return coreStr[i]; }
    char& at(size_t i)               { return coreStr.at(i); }    // NEW: bounds-checked
    const char& at(size_t i)         const { return coreStr.at(i); }

    char front() const { return coreStr.front(); }  // NEW
    char back()  const { return coreStr.back();  }  // NEW

    // ── Iterators ─────────────────────────────────────────────────────────────

    auto begin()        { return coreStr.begin(); }
    auto end()          { return coreStr.end();   }
    auto begin()  const { return coreStr.begin(); }
    auto end()    const { return coreStr.end();   }
    auto cbegin() const { return coreStr.cbegin(); }
    auto cend()   const { return coreStr.cend();   }
    auto rbegin()       { return coreStr.rbegin(); }
    auto rend()         { return coreStr.rend();   }

    // ── Capacity ──────────────────────────────────────────────────────────────

    size_t length() const { return coreStr.length(); }
    size_t size()   const { return coreStr.size(); }   // NEW: alias
    bool   empty()  const { return coreStr.empty(); }
    void   clear()        { coreStr.clear(); }
    void   reserve(size_t n)  { coreStr.reserve(n); }  // NEW
    void   resize(size_t n, char c = '\0') { coreStr.resize(n, c); } // NEW

    // ── Search ────────────────────────────────────────────────────────────────

    // Returns std::string::npos when not found (same as std::string).
    size_t find(const SString& s, size_t pos = 0)      const { return coreStr.find(s.coreStr, pos); }
    size_t findLast(const SString& s)                   const { return coreStr.rfind(s.coreStr); }       // NEW
    size_t findFirst(char c, size_t pos = 0)            const { return coreStr.find_first_of(c, pos); }  // NEW
    size_t findLast(char c)                             const { return coreStr.find_last_of(c); }         // NEW

    [[nodiscard]] bool contains(const SString& s) const   // NEW
    {
        return coreStr.find(s.coreStr) != std::string::npos;
    }

    [[nodiscard]] bool startsWith(const SString& prefix) const  // NEW
    {
        return coreStr.size() >= prefix.coreStr.size() &&
               coreStr.compare(0, prefix.coreStr.size(), prefix.coreStr) == 0;
    }

    [[nodiscard]] bool endsWith(const SString& suffix) const    // NEW
    {
        return coreStr.size() >= suffix.coreStr.size() &&
               coreStr.compare(coreStr.size() - suffix.coreStr.size(),
                               suffix.coreStr.size(), suffix.coreStr) == 0;
    }

    // ── Extraction / manipulation ─────────────────────────────────────────────

    [[nodiscard]] SString substring(size_t pos, size_t len = std::string::npos) const
    {
        return coreStr.substr(pos, len);
    }

    [[nodiscard]] std::vector<SString> split(const SString& delim) const
    {
        std::vector<SString> result;
        size_t start = 0, end = 0;
        while ((end = coreStr.find(delim.coreStr, start)) != std::string::npos)
        {
            result.emplace_back(coreStr.substr(start, end - start));
            start = end + delim.coreStr.size();
        }
        result.emplace_back(coreStr.substr(start));
        return result;
    }

    // NEW: join a vector of SStrings with this string as separator
    static SString join(const SString& sep, const std::vector<SString>& parts)
    {
        std::string result;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (i > 0) result += sep.coreStr;
            result += parts[i].coreStr;
        }
        return result;
    }

    // ── In-place mutation ─────────────────────────────────────────────────────

    void toUpperCase()
    {
        std::transform(coreStr.begin(), coreStr.end(), coreStr.begin(),
                       [](unsigned char c){ return std::toupper(c); });
    }

    void toLowerCase()
    {
        std::transform(coreStr.begin(), coreStr.end(), coreStr.begin(),
                       [](unsigned char c){ return std::tolower(c); });
    }

    void trim()
    {
        coreStr.erase(coreStr.begin(),
            std::find_if(coreStr.begin(), coreStr.end(),
                         [](unsigned char c){ return !std::isspace(c); }));
        coreStr.erase(
            std::find_if(coreStr.rbegin(), coreStr.rend(),
                         [](unsigned char c){ return !std::isspace(c); }).base(),
            coreStr.end());
    }

    void replace(const SString& target, const SString& replacement)
    {
        replaceAll(coreStr, target.coreStr, replacement.coreStr);
    }

    void insert(size_t pos, const SString& s)  // NEW
    {
        coreStr.insert(pos, s.coreStr);
    }

    void erase(size_t pos, size_t len = std::string::npos)  // NEW
    {
        coreStr.erase(pos, len);
    }

    void reverse()  // NEW
    {
        std::reverse(coreStr.begin(), coreStr.end());
    }

    void append(const SString& s) { coreStr.append(s.coreStr); }

    // ── Non-mutating versions (return new SString) ────────────────────────────

    [[nodiscard]] SString toUpper()   const { SString s(*this); s.toUpperCase(); return s; }  // NEW
    [[nodiscard]] SString toLower()   const { SString s(*this); s.toLowerCase(); return s; }  // NEW
    [[nodiscard]] SString trimmed()   const { SString s(*this); s.trim(); return s; }         // NEW
    [[nodiscard]] SString reversed()  const { SString s(*this); s.reverse(); return s; }      // NEW

    [[nodiscard]] SString replaced(const SString& target,
                                   const SString& replacement) const  // NEW
    {
        SString s(*this);
        s.replace(target, replacement);
        return s;
    }

    // ── Padding ───────────────────────────────────────────────────────────────

    [[nodiscard]] SString padLeft(size_t totalWidth, char padChar = ' ') const  // NEW
    {
        if (coreStr.size() >= totalWidth) return *this;
        return SString(std::string(totalWidth - coreStr.size(), padChar) + coreStr);
    }

    [[nodiscard]] SString padRight(size_t totalWidth, char padChar = ' ') const  // NEW
    {
        if (coreStr.size() >= totalWidth) return *this;
        return SString(coreStr + std::string(totalWidth - coreStr.size(), padChar));
    }

    // ── Repeat ───────────────────────────────────────────────────────────────

    [[nodiscard]] SString repeat(size_t times) const  // NEW
    {
        std::string result;
        result.reserve(coreStr.size() * times);
        for (size_t i = 0; i < times; ++i) result += coreStr;
        return result;
    }

    // ── Stream operators ──────────────────────────────────────────────────────

    friend std::ostream& operator<<(std::ostream& os, const SString& s)
    {
        return os << s.coreStr;
    }
    friend std::istream& operator>>(std::istream& is, SString& s)
    {
        return is >> s.coreStr;
    }

    // ── Hash (for use as unordered_map key directly) ──────────────────────────

    struct Hash
    {
        std::size_t operator()(const SString& s) const noexcept
        {
            return std::hash<std::string>{}(s.coreStr);
        }
    };
};

// ── std::hash specialisation ──────────────────────────────────────────────────
// Allows SString to be used directly as unordered_map / unordered_set key
// without specifying a custom hasher.

template<>
struct std::hash<SString>
{
    std::size_t operator()(const SString& s) const noexcept
    {
        return std::hash<std::string>{}(s.str());
    }
};