//
// particle_emitter_inspector_drawer.cpp
//

#include "particle_emitter_inspector_drawer.h"

#include <cctype>
#include <string>

#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/particles/particle_emitter.h"
#include "core/engine/particles/particle_registry.h"
#include "core/engine/texture/textureasset.h"
#include "core/memory/mnew.h"
#include "core/utils/curve.h"
#include "core/utils/field_engine_types.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/editorwindows/inspectordrawer/controls/curve_gradient_controls.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"

const bool MParticleEmitterInspectorDrawer::registered = []() {
    registerDrawer(mnew<MParticleEmitterInspectorDrawer>());
    return true;
}();

bool MParticleEmitterInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MParticleEmitter*>(entity) != nullptr;
}

namespace
{
    // "scrollSpeed" -> "Scroll Speed"
    std::string prettyFieldName(const std::string& name)
    {
        std::string out;
        for (size_t i = 0; i < name.size(); ++i)
        {
            const char c = name[i];
            if (i == 0)                                            out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            else if (std::isupper(static_cast<unsigned char>(c))) { out += ' '; out += c; }
            else                                                   out += c;
        }
        return out;
    }

    const char* const SHAPE_NAMES[] = { "Point", "Square", "Circle", "Box", "Sphere", "Cone" };
    const char* const SPACE_NAMES[] = { "Local", "Global" };
}

void MParticleEmitterInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    MSpatialEntityInspectorDrawer::onDrawInspector(target);   // transform
    auto* emitter = dynamic_cast<MParticleEmitter*>(target);
    if (!emitter) return;

    ImGui::PushID("particleEmitter");
    if (ImGui::CollapsingHeader("Particle Emitter", ImGuiTreeNodeFlags_DefaultOpen)) drawEmitter(emitter);
    if (ImGui::CollapsingHeader("Shape",            ImGuiTreeNodeFlags_DefaultOpen)) drawShape(emitter);
    if (ImGui::CollapsingHeader("Renderer",         ImGuiTreeNodeFlags_DefaultOpen)) drawRenderer(emitter);
    if (ImGui::CollapsingHeader("Modules",          ImGuiTreeNodeFlags_DefaultOpen)) drawModules(emitter);
    ImGui::PopID();
}

// -----------------------------------------------------------------------------
// Emitter
// -----------------------------------------------------------------------------

void MParticleEmitterInspectorDrawer::drawEmitter(MParticleEmitter* e)
{
    START_FIELD_DRAW("Particles", "Alive now / pool size (Max Particles).")
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%d / %d", e->getAliveCount(), e->maxParticles.get());
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) e->clear();
        ImGui::SameLine();
        if (ImGui::SmallButton("Burst 10")) e->emit(10);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Auto Simulate", "Step every frame. Off: only simulate(dt) from a script moves it.")
        bool v = e->autoSimulate.get();
        if (ImGui::Checkbox("##v", &v)) e->autoSimulate.set(v);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Max Particles", "Pool size — memory is allocated for this many up front.\n"
                                      "New particles are dropped while the pool is full.")
        int v = e->maxParticles.get();
        if (MInspectorField::DragInt("##v", &v, 1.0f, 1, 100000)) e->maxParticles.set(std::max(1, v));
    END_FIELD_DRAW()

    START_FIELD_DRAW("Emission Rate", "New particles per second.")
        float v = e->emissionRate.get();
        if (MInspectorField::DragFloat("##v", &v, 0.1f, 0.0f, 100000.0f, "%.1f")) e->emissionRate.set(std::max(0.0f, v));
    END_FIELD_DRAW()

    START_FIELD_DRAW("Simulation Space", "Local: particles move with the emitter.\nGlobal: particles stay where they were emitted.")
        int v = static_cast<int>(e->simulationSpace.get());
        if (ImGui::Combo("##v", &v, SPACE_NAMES, IM_ARRAYSIZE(SPACE_NAMES)))
        {
            e->simulationSpace.set(static_cast<EParticleSimulationSpace>(v));
            e->clear();   // existing positions are in the old space
        }
    END_FIELD_DRAW()

    MInspectorField::headerRow("Start");

    START_FIELD_DRAW("Lifetime", "Seconds each particle lives.")
        float v = e->startLifetime.get();
        if (MInspectorField::DragFloat("##v", &v, 0.05f, 0.01f, 1000.0f, "%.2f")) e->startLifetime.set(v);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Speed", "Initial speed along the shape's direction.")
        float v = e->startSpeed.get();
        if (MInspectorField::DragFloat("##v", &v, 0.05f)) e->startSpeed.set(v);
    END_FIELD_DRAW()

    START_FIELD_DRAW("3D Size", "Off: one size for every axis.\nOn: separate X / Y / Z (sprites use X and Y).")
        bool v = e->startSize3D.get();
        if (ImGui::Checkbox("##v", &v)) e->startSize3D.set(v);
    END_FIELD_DRAW()

    if (e->startSize3D.get())
    {
        START_FIELD_DRAW("Size", "Start size per axis. Size Over Lifetime scales all three together.")
            SVector3 v = e->startSizeXYZ.get();
            if (ImGui::DragFloat3("##v", &v.x, 0.01f, 0.0f, 1000.0f)) e->startSizeXYZ.set(glm::max(v, SVector3(0.0f)));
        END_FIELD_DRAW()
    }
    else
    {
        START_FIELD_DRAW("Size")
            float v = e->startSize.get();
            if (MInspectorField::DragFloat("##v", &v, 0.01f, 0.0f, 1000.0f)) e->startSize.set(v);
        END_FIELD_DRAW()
    }

    START_FIELD_DRAW("Color", "Start colour and alpha. Color Over Lifetime multiplies this.")
        const SVector3 c = e->startColor.get();
        float rgba[4] = { c.x, c.y, c.z, e->startAlpha.get() };
        if (ImGui::ColorEdit4("##v", rgba, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar))
        {
            e->startColor.set(SVector3(rgba[0], rgba[1], rgba[2]));
            e->startAlpha.set(rgba[3]);
        }
    END_FIELD_DRAW()
}

// -----------------------------------------------------------------------------
// Shape
// -----------------------------------------------------------------------------

void MParticleEmitterInspectorDrawer::drawShape(MParticleEmitter* e)
{
    START_FIELD_DRAW("Shape", "Where particles start. They are emitted along local +Y,\nor the Velocity module's Direction when it has one.")
        int v = static_cast<int>(e->emissionShape.get());
        if (ImGui::Combo("##v", &v, SHAPE_NAMES, IM_ARRAYSIZE(SHAPE_NAMES)))
            e->emissionShape.set(static_cast<EParticleEmitterShape>(v));
    END_FIELD_DRAW()

    const EParticleEmitterShape shape = e->emissionShape.get();

    if (shape == EParticleEmitterShape::Circle || shape == EParticleEmitterShape::Sphere || shape == EParticleEmitterShape::Cone)
    {
        START_FIELD_DRAW("Radius")
            float v = e->shapeRadius.get();
            if (MInspectorField::DragFloat("##v", &v, 0.01f, 0.0f, 10000.0f)) e->shapeRadius.set(v);
        END_FIELD_DRAW()
    }

    if (shape == EParticleEmitterShape::Cone)
    {
        START_FIELD_DRAW("Angle", "Degrees from +Y at the rim of the cone. 0 = straight up.")
            float v = e->shapeAngle.get();
            if (MInspectorField::SliderFloat("##v", &v, 0.0f, 89.0f, "%.1f")) e->shapeAngle.set(v);
        END_FIELD_DRAW()
    }

    if (shape == EParticleEmitterShape::Square || shape == EParticleEmitterShape::Box)
    {
        START_FIELD_DRAW("Extents", shape == EParticleEmitterShape::Square ? "Half sizes. Square uses X and Z." : "Half sizes.")
            SVector3 v = e->shapeExtents.get();
            if (ImGui::DragFloat3("##v", &v.x, 0.01f, 0.0f, 10000.0f)) e->shapeExtents.set(glm::max(v, SVector3(0.0f)));
        END_FIELD_DRAW()
    }
}

// -----------------------------------------------------------------------------
// Renderer
// -----------------------------------------------------------------------------

void MParticleEmitterInspectorDrawer::drawRenderer(MParticleEmitter* e)
{
    IParticleRenderer* renderer = e->getRenderer();
    const SString current = renderer ? renderer->getTypeName() : SString("None");

    START_FIELD_DRAW("Type", "Sprite: camera-facing quads. Mesh: a mesh per particle.")
        if (ImGui::BeginCombo("##v", current.c_str()))
        {
            if (ImGui::Selectable("None", renderer == nullptr))
                e->setRenderer({});
            for (const auto& entry : MParticleRendererRegistry::getAll())
                if (ImGui::Selectable(entry.name.c_str(), renderer && entry.name == current))
                    e->setRenderer(entry.name);
            ImGui::EndCombo();
        }
    END_FIELD_DRAW()

    // The combo may have replaced it — fetch again.
    if (IParticleRenderer* r = e->getRenderer())
    {
        ImGui::PushID("renderer");
        drawSettingsFields(r);
        ImGui::PopID();
    }
}

// -----------------------------------------------------------------------------
// Modules
// -----------------------------------------------------------------------------

void MParticleEmitterInspectorDrawer::drawModules(MParticleEmitter* e)
{
    SString toRemove;
    for (IParticleSimulator* sim : e->getSimulators())
    {
        const SString& type = sim->getTypeName();
        ImGui::PushID(type.c_str());

        bool enabled = sim->enabled.get();
        if (ImGui::Checkbox("##enabled", &enabled)) sim->enabled.set(enabled);
        ImGui::SameLine();

        const bool open = ImGui::TreeNodeEx(type.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
        if (ImGui::BeginPopupContextItem("##moduleMenu"))
        {
            if (ImGui::MenuItem("Remove")) toRemove = type;
            ImGui::EndPopup();
        }

        if (open)
        {
            if (!enabled) ImGui::BeginDisabled();
            drawSettingsFields(sim);
            if (!enabled) ImGui::EndDisabled();

            if (ImGui::SmallButton("Remove")) toRemove = type;
            ImGui::TreePop();
        }

        ImGui::PopID();
        ImGui::Spacing();
    }

    // Removing deletes the module — only after the loop.
    if (!toRemove.empty())
        e->removeSimulator(toRemove);

    if (e->getSimulator("Velocity") == nullptr)
        MInspectorField::statusText(EInspectorStatus::Warning, "No Velocity module — particles won't move.");

    ImGui::Separator();
    if (ImGui::Button("Add Module", ImVec2(-1, 0)))
        ImGui::OpenPopup("##addParticleModule");

    if (ImGui::BeginPopup("##addParticleModule"))
    {
        bool any = false;
        for (const auto& entry : MParticleSimulatorRegistry::getAll())
        {
            if (e->getSimulator(entry.name)) continue;   // one per type
            any = true;
            if (ImGui::Selectable(entry.name.c_str()))
                e->addSimulator(entry.name);
        }
        if (!any) ImGui::TextDisabled("All modules added");
        ImGui::EndPopup();
    }
}

// -----------------------------------------------------------------------------
// Generic settings
// -----------------------------------------------------------------------------

MAssetReferenceControl* MParticleEmitterInspectorDrawer::getAssetControl(const FieldBase* field, EAssetKind kind)
{
    auto it = assetControls.find(field);
    if (it != assetControls.end()) return it->second;

    auto* control = mnew<MAssetReferenceControl>();
    if (kind == EAssetKind::Texture)
        control->canAcceptAssetFuncCallback = [](const TAssetHandle<MAsset>& asset)
        { return dynamic_cast<MTextureAsset*>(asset.get()) != nullptr; };
    else
        control->canAcceptAssetFuncCallback = [](const TAssetHandle<MAsset>& asset)
        { return dynamic_cast<MStaticMeshAsset*>(asset.get()) != nullptr; };

    assetControls.emplace(field, control);
    return control;
}

void MParticleEmitterInspectorDrawer::drawSettingsFields(IParticleSettings* settings)
{
    for (FieldBase* field : settings->getFields())
    {
        if (field->name == "enabled") continue;   // drawn in the module header

        const SParticleFieldMeta meta  = settings->getFieldMeta(field->name);
        const std::string        label = prettyFieldName(field->name);
        const char*              tip   = meta.tooltip;

        if (auto* f = dynamic_cast<Field<float>*>(field))
        {
            START_FIELD_DRAW(label, tip)
                float v = f->get();
                const bool changed = (meta.max > meta.min)
                    ? MInspectorField::SliderFloat("##v", &v, meta.min, meta.max, "%.2f")
                    : MInspectorField::DragFloat("##v", &v, meta.speed);
                if (changed) f->set(v);
            END_FIELD_DRAW()
        }
        else if (auto* f = dynamic_cast<Field<int>*>(field))
        {
            START_FIELD_DRAW(label, tip)
                int v = f->get();
                const bool changed = !meta.options.empty()
                    ? ImGui::Combo("##v", &v, meta.options.data(), static_cast<int>(meta.options.size()))
                    : MInspectorField::DragInt("##v", &v, 1.0f, static_cast<int>(meta.min), static_cast<int>(meta.max));
                if (changed) f->set(v);
            END_FIELD_DRAW()
        }
        else if (auto* f = dynamic_cast<Field<bool>*>(field))
        {
            START_FIELD_DRAW(label, tip)
                bool v = f->get();
                if (ImGui::Checkbox("##v", &v)) f->set(v);
            END_FIELD_DRAW()
        }
        else if (auto* f = dynamic_cast<Field<SVector3>*>(field))
        {
            START_FIELD_DRAW(label, tip)
                SVector3 v = f->get();
                const bool changed = meta.color
                    ? ImGui::ColorEdit3("##v", &v.x, ImGuiColorEditFlags_Float)
                    : ImGui::DragFloat3("##v", &v.x, meta.speed);
                if (changed) f->set(v);
            END_FIELD_DRAW()
        }
        else if (auto* f = dynamic_cast<Field<SCurve>*>(field))
        {
            START_FIELD_DRAW(label, tip)
                SCurve v = f->get();   // edit a copy — the simulation reads the field every frame
                if (MCurveGradientControls::curveField("##v", v, meta.curveMin, meta.curveMax)) f->set(v);
            END_FIELD_DRAW()
        }
        else if (auto* f = dynamic_cast<Field<SGradient>*>(field))
        {
            START_FIELD_DRAW(label, tip)
                SGradient v = f->get();
                if (MCurveGradientControls::gradientField("##v", v)) f->set(v);
            END_FIELD_DRAW()
        }
        else if (auto* f = dynamic_cast<Field<TAssetRef<MTextureAsset>>*>(field))
        {
            MAssetReferenceControl* control = getAssetControl(field, EAssetKind::Texture);
            const auto& cur = f->get();
            if (control->getAssetReference().getAssetId() != cur.getAssetId())
                control->setAssetReference(cur.getHandle());

            START_FIELD_DRAW(label, tip)
                if (control->drawControl(""))
                {
                    TAssetHandle<MTextureAsset> handle = control->getAssetReference();
                    f->set(TAssetRef<MTextureAsset>(handle));
                }
            END_FIELD_DRAW()
        }
        else if (auto* f = dynamic_cast<Field<TAssetRef<MStaticMeshAsset>>*>(field))
        {
            MAssetReferenceControl* control = getAssetControl(field, EAssetKind::Mesh);
            const auto& cur = f->get();
            if (control->getAssetReference().getAssetId() != cur.getAssetId())
                control->setAssetReference(cur.getHandle());

            START_FIELD_DRAW(label, tip)
                if (control->drawControl(""))
                {
                    TAssetHandle<MStaticMeshAsset> handle = control->getAssetReference();
                    f->set(TAssetRef<MStaticMeshAsset>(handle));
                }
            END_FIELD_DRAW()
        }
    }
}
