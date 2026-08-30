// serialized_class_base.h  (STUB for binding generator parsing)
#ifndef SERIALISEDCLASSBASE_H
#define SERIALISEDCLASSBASE_H

#include "std_stub.h"
#include "field.h"
#include "list_field.h"

#define DECLARE_FIELD(fieldName, type, ...) \
    Field<type> fieldName;

class SerializedClassBase {
protected:
    std::vector<FieldBase*> fields;
public:
    virtual ~SerializedClassBase() = default;
};

#endif // SERIALISEDCLASSBASE_H
