#include "sprite_inspector_drawer.h"

#include "core/engine/sprite/sprite_entity.h"
#include "core/engine/texture/textureasset.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/memory/mnew.h"

bool MSpriteInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MSpriteInspectorDrawer>());
    return true;
}();

MSpriteInspectorDrawer::MSpriteInspectorDrawer()
{
    textureControl = mnew<MAssetReferenceControl>();
    textureControl->canAcceptAssetFuncCallback = [](const TAssetHandle<MAsset>& asset)
    { return dynamic_cast<MTextureAsset*>(asset.get()) != nullptr; };
}

bool MSpriteInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MSpriteEntity*>(entity) != nullptr;
}

void MSpriteInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    auto* sprite = dynamic_cast<MSpriteEntity*>(target);
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    if (!sprite) return;

    if (!ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // -- Texture --
    const auto cur = sprite->getTextureRef();
    if (textureControl->getAssetReference().getAssetId() != cur.getAssetId())
        textureControl->setAssetReference(cur.getHandle());

    START_FIELD_DRAW("Texture")
        if (textureControl->drawControl(""))
        {
            TAssetHandle<MTextureAsset> tex = textureControl->getAssetReference();
            sprite->setTexture(tex);
        }
    END_FIELD_DRAW()

    START_FIELD_DRAW("Color", "Tint, multiplied with the texture. Alpha is Opacity.")
        SVector3 c = sprite->getColor();
        float rgba[4] = { c.x, c.y, c.z, sprite->getOpacity() };
        if (ImGui::ColorEdit4("##v", rgba, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar))
        {
            sprite->setColor(SVector3(rgba[0], rgba[1], rgba[2]));
            sprite->setOpacity(rgba[3]);
        }
    END_FIELD_DRAW()

    START_FIELD_DRAW("Lit", "Receive scene lighting and shadows. Off draws the texture at full brightness.")
        bool lit = sprite->getLit();
        if (ImGui::Checkbox("##v", &lit)) sprite->setLit(lit);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Pixels Per Unit", "Texture pixels per world unit - sets the quad's size.")
        float ppu = sprite->getPixelsPerUnit();
        if (MInspectorField::DragFloat("##v", &ppu, 1.0f, 0.001f, 100000.0f, "%.1f"))
            sprite->setPixelsPerUnit(ppu);
    END_FIELD_DRAW()

    const SVector2 size = sprite->getSize();
    START_FIELD_DRAW("Size", "Quad size in local units, before the transform's scale.")
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%.3g x %.3g", size.x, size.y);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Flip")
        bool fx = sprite->getFlipX(), fy = sprite->getFlipY();
        if (ImGui::Checkbox("X##flipx", &fx)) sprite->setFlipX(fx);
        ImGui::SameLine();
        if (ImGui::Checkbox("Y##flipy", &fy)) sprite->setFlipY(fy);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Transparent",
                     "Off: opaque with alpha cutout - hard edges, casts shadows, no sorting issues.\n"
                     "On: alpha-blended - soft edges, sorted back to front, no shadows.")
        bool tr = sprite->getTransparent();
        if (ImGui::Checkbox("##v", &tr)) sprite->setTransparent(tr);
    END_FIELD_DRAW()

    ImGui::BeginDisabled(sprite->getTransparent());

    START_FIELD_DRAW("Cutout Threshold", "Pixels with alpha below this are not drawn.")
        float t = sprite->getCutoutThreshold();
        if (MInspectorField::SliderFloat("##v", &t, 0.0f, 1.0f, "%.2f"))
            sprite->setCutoutThreshold(t);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Casts Shadow")
        bool cs = sprite->getCastsShadow();
        if (ImGui::Checkbox("##v", &cs)) sprite->setCastsShadow(cs);
    END_FIELD_DRAW()

    ImGui::EndDisabled();
}
