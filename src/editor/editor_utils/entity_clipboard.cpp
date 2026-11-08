//
// entity_clipboard.cpp
//

#include "entity_clipboard.h"

#include <algorithm>

#include "imgui.h"

#include "entity_duplicator.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "core/utils/logger.h"
#include "editor/app/editorapplication.h"

MSpatialEntity* MEntityClipboard::cutSource = nullptr;
MScene*         MEntityClipboard::cutScene  = nullptr;
std::string     MEntityClipboard::cutText;

namespace
{
MScene* activeScene()
{
    auto* sceneManager = MEngineSubsystemRegistry::getSubsystem<MSceneManager>();
    return sceneManager ? sceneManager->getActiveScene() : nullptr;
}

std::string clipboardText()
{
    if (!ImGui::GetCurrentContext()) return {};
    const char* text = ImGui::GetClipboardText();
    return text ? std::string(text) : std::string();
}

bool isSelfOrDescendant(MSpatialEntity* entity, const MSpatialEntity* ancestor)
{
    for (auto e = entity; e; e = e->getParent())
        if (e == ancestor) return true;
    return false;
}

int indexOf(const std::vector<MSpatialEntity*>& list, const MSpatialEntity* entity)
{
    const auto it = std::ranges::find(list, entity);
    return it == list.end() ? -1 : static_cast<int>(std::distance(list.begin(), it));
}

MSpatialEntity* selectedEntity()
{
    return dynamic_cast<MSpatialEntity*>(MEditorApplication::SelectedObject);
}
} // namespace

// ---- Cut bookkeeping -------------------------------------------------------------

MSpatialEntity* MEntityClipboard::liveCutSource()
{
    if (!cutSource) return nullptr;

    // The pointer is only trusted while the scene still lists it: the source
    // may have been deleted, or the scene changed, since the cut.
    MScene* scene = activeScene();
    if (scene != cutScene || !scene || !scene->getAllEntities().contains(cutSource))
    {
        clearCut();
        return nullptr;
    }
    return cutSource;
}

void MEntityClipboard::clearCut()
{
    cutSource = nullptr;
    cutScene  = nullptr;
    cutText.clear();
}

bool MEntityClipboard::isPendingCut(MSpatialEntity* entity)
{
    if (!cutSource || !entity) return false;
    // Cheap: no scene lookup. A stale cutSource can't match a live entity
    // unless the address was reused, and then the worst case is a dim row.
    return isSelfOrDescendant(entity, cutSource);
}

// ---- Copy / cut ---------------------------------------------------------------------

bool MEntityClipboard::copy(MSpatialEntity* source)
{
    const SString text = MEntityDuplicator::serialiseToText(source);
    if (text.empty()) return false;

    ImGui::SetClipboardText(text.c_str());
    clearCut();   // a new copy replaces any pending cut
    MLOG(SString("Copy:: ") + source->getName());
    return true;
}

bool MEntityClipboard::cut(MSpatialEntity* source)
{
    const SString text = MEntityDuplicator::serialiseToText(source);
    if (text.empty()) return false;

    ImGui::SetClipboardText(text.c_str());
    cutSource = source;
    cutScene  = activeScene();
    cutText   = text.str();
    MLOG(SString("Cut:: ") + source->getName());
    return true;
}

// ---- Paste ----------------------------------------------------------------------------

bool MEntityClipboard::canPaste()
{
    return MEntityDuplicator::isEntityText(SString(clipboardText().c_str()));
}

MSpatialEntity* MEntityClipboard::paste(MSpatialEntity* anchor, bool asChild)
{
    const std::string text = clipboardText();
    if (!MEntityDuplicator::isEntityText(SString(text.c_str())))
        return nullptr;

    // Only a paste of exactly what was cut moves the source.
    MSpatialEntity* source = liveCutSource();
    if (source && text != cutText)
    {
        clearCut();
        source = nullptr;
    }

    // Pasting a cut entity into itself would delete the paste along with
    // the source - put it next to the source instead.
    if (source && anchor && isSelfOrDescendant(anchor, source) && (asChild || anchor != source))
    {
        anchor  = source;
        asChild = false;
    }

    MSpatialEntity* parent = nullptr;
    int             index  = -1;
    if (anchor && asChild)
    {
        parent = anchor;
    }
    else if (anchor)
    {
        parent = anchor->getParent();
        if (parent)
            index = indexOf(parent->getChildren(), anchor) + 1;
        else if (MScene* scene = activeScene())
            index = indexOf(scene->getRootEntities(), anchor) + 1;
        if (index == 0) index = -1;   // anchor not found: append
    }

    const auto created = MEntityDuplicator::instantiateFromText(SString(text.c_str()), parent, index);
    if (created.empty())
        return nullptr;

    // Keep the copied name unless a sibling already has it (the cut source
    // doesn't count - it's about to go).
    for (auto* e : created)
        MEntityDuplicator::makeNameUnique(e, true, source);

    if (source)
    {
        // Deselect before deleting, or the inspector keeps a freed pointer.
        if (auto* selected = selectedEntity(); selected && isSelfOrDescendant(selected, source))
            MEditorApplication::SelectedObject = nullptr;
        MLOG(SString("Cut:: moved ") + source->getName());
        source->destroy();
        clearCut();   // later pastes of the same text are plain copies
    }

    MEditorApplication::SelectedObject = created.front();
    return created.front();
}

// ---- Selection helpers ------------------------------------------------------------------

void MEntityClipboard::copySelected()     { copy(selectedEntity()); }
void MEntityClipboard::cutSelected()      { cut(selectedEntity()); }
void MEntityClipboard::pasteAtSelection() { paste(selectedEntity(), false); }
