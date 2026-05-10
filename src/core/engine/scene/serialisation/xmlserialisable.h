//
// Created by ssj5v on 26-09-2024.
//

#ifndef XMLSERIALISABLE_H
#define XMLSERIALISABLE_H

#include <pugixml.hpp>
#include "core/meteor_core_minimal.h"

class IMXmlSerializable {
public:
    virtual ~IMXmlSerializable() = default;
    virtual pugi::xml_document* serializeToXml()=0;
};

#endif //XMLSERIALISABLE_H
