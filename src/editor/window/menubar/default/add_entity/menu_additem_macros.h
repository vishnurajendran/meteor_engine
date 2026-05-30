#pragma once
#include "editor/window/menubar/menubartree.h"

#if METEOR_EDITOR

#define REGISTER_SPATIAL_ENTITY_CREATE_MENU(ClassName, MenuPath, PriorityValue, EntityType, EntityName) \
class ClassName : public MMenubarItem {                                                                 \
DEFINE_OBJECT_SUBCLASS(ClassName)                                                                       \
public:                                                                                                 \
[[nodiscard]] int getPriority() const override { return PriorityValue; }                                \
[[nodiscard]] SString getPath() const override { return MenuPath; }                                     \
void onSelect() override                                                                                \
{                                                                                                       \
MSpatialEntity::createInstance<EntityType>(EntityName);                                                 \
}                                                                                                       \
private:                                                                                                \
static bool registered;                                                                                 \
};                                                                                                      \
                                                                                                        \
bool ClassName::registered = []() {                                                                     \
MMenubarTreeNode::registerItem(new ClassName());                                                        \
return true;                                                                                            \
}();
#endif