// std_stub.h  (STUB for binding generator parsing)
//
// Provides minimal stand-ins for C++ standard library types so libclang
// can parse engine headers without knowing where MinGW/MSVC/libc++
// installed the real standard headers.
//
// These are NOT real implementations. They exist only so the AST parser
// sees the correct type names and template structures.

#ifndef STD_STUB_H
#define STD_STUB_H

namespace std {

class string {
public:
    string() {}
    string(const char*) {}
    const char* c_str() const { return ""; }
    bool empty() const { return true; }
    int size() const { return 0; }
};

template<typename T, typename Alloc = void>
class vector {
public:
    vector() {}
    T* begin();
    T* end();
    int size() const { return 0; }
    bool empty() const { return true; }
    void push_back(const T&) {}
    T& operator[](int) { static T t; return t; }
    const T& operator[](int) const { static T t; return t; }
};

template<typename T>
class function {};

template<typename R, typename... Args>
class function<R(Args...)> {
public:
    function() {}
    operator bool() const { return false; }
};

template<typename T>
class unique_ptr {
public:
    T* get() const { return nullptr; }
    T* operator->() const { return nullptr; }
};

template<typename T>
class shared_ptr {
public:
    T* get() const { return nullptr; }
    T* operator->() const { return nullptr; }
};

// Minimal type_traits stubs
template<typename T, typename U> struct is_same { static constexpr bool value = false; };
template<typename T> struct is_same<T,T> { static constexpr bool value = true; };
template<typename T, typename U> inline constexpr bool is_same_v = is_same<T,U>::value;

template<typename T> struct is_enum { static constexpr bool value = false; };
template<typename T> inline constexpr bool is_enum_v = is_enum<T>::value;

template<typename Base, typename Derived> struct is_base_of { static constexpr bool value = false; };

template<typename T> struct remove_reference { using type = T; };
template<typename T> struct remove_reference<T&> { using type = T; };
template<typename T> struct remove_reference<T&&> { using type = T; };

// Minimal algorithm/utility
template<typename T> const T& min(const T& a, const T& b) { return a < b ? a : b; }
template<typename T> const T& max(const T& a, const T& b) { return a > b ? a : b; }

// find for vector iteration
template<typename It, typename T>
It find(It first, It last, const T& val) { return first; }

} // namespace std

// size_t
typedef unsigned long long size_t;

// nullptr_t
namespace std { typedef decltype(nullptr) nullptr_t; }

#endif // STD_STUB_H
