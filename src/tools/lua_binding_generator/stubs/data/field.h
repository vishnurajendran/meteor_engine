// field.h  (STUB for binding generator parsing)
#ifndef FIELD_H
#define FIELD_H

#include "field_base.h"

template<typename T>
struct Field : public FieldBase {
    T rawValue;
    const T& get() const { return rawValue; }
    T&       get()       { return rawValue; }
    void     set(const T& v) { rawValue = v; }
    Field& operator=(const T& v) { rawValue = v; return *this; }
};

#endif // FIELD_H
