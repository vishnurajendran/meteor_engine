//
// Created by ssj5v on 01-06-2026.
//

#include "open_documentation.h"

#include "editor/window/menubar/menubartree.h"

bool MOpenDocumentation::registerd = []()
{
    MMenubarTreeNode::registerItem(new MOpenDocumentation());
    return true;
}();