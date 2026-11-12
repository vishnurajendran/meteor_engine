//
// entity_clipboard.h
//
// Copy / cut / paste of entities, built on the two halves of
// MEntityDuplicator: copy puts the entity's scene XML on the system
// clipboard (so it can be pasted into another editor instance), paste
// builds entities from it.
//
// Cut copies and remembers the source; the source is only deleted once a
// paste succeeds, and only if the clipboard still holds what was cut.
//

#ifndef ENTITY_CLIPBOARD_H
#define ENTITY_CLIPBOARD_H

#include <string>

class MSpatialEntity;
class MScene;

class MEntityClipboard
{
public:
    static bool copy(MSpatialEntity* source);
    static bool cut(MSpatialEntity* source);

    // Pastes and selects the result. With `anchor`:
    //   asChild = false - as a sibling right after `anchor`
    //   asChild = true  - as the last child of `anchor`
    // Without one, at the end of the scene root.
    // Returns the first pasted entity, or nullptr.
    static MSpatialEntity* paste(MSpatialEntity* anchor, bool asChild = false);

    // Clipboard holds entities. Reads (and parses) the system clipboard -
    // call it when drawing a menu, not every frame.
    [[nodiscard]] static bool canPaste();

    // True for the entity waiting to be moved by a pending cut (or one of its
    // descendants) - lets the hierarchy dim it.
    [[nodiscard]] static bool isPendingCut(MSpatialEntity* entity);

    // Act on MEditorApplication::SelectedObject (Edit menu / shortcuts).
    static void copySelected();
    static void cutSelected();
    static void pasteAtSelection();

private:
    static MSpatialEntity* liveCutSource();   // nullptr once deleted or no cut pending
    static void            clearCut();

    static MSpatialEntity* cutSource;
    static MScene*         cutScene;
    static std::string     cutText;
};

#endif // ENTITY_CLIPBOARD_H
