// list_field.h  (STUB for binding generator parsing)
#ifndef LIST_FIELD_H
#define LIST_FIELD_H

#include "field_base.h"

template<typename T>
class ListField : public FieldBase {
public:
    const std::vector<T>& get() const;
    void set(const std::vector<T>& v);
    void add(const T& item);
};

#define DECLARE_LIST_FIELD(fieldName, type, ...) \
    ListField<type> fieldName;

#endif // LIST_FIELD_H
