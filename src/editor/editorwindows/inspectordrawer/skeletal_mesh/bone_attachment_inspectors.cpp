#include "bone_attachment_inspectors.h"

#include "bone_picker.h"
#include "core/engine/3d/skeletal/bone_control_entity.h"
#include "core/engine/3d/skeletal/bone_socket_entity.h"
#include "core/engine/3d/skeletal/skeletal_mesh_entity.h"
#include "core/memory/mnew.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"

bool MBoneSocketInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MBoneSocketInspectorDrawer>());
    return true;
}();

bool MBoneControlInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MBoneControlInspectorDrawer>());
    return true;
}();

namespace
{
    // Shared: parent check + bone dropdown. Returns true if the bone changed.
    bool drawBoneRow(MSpatialEntity* target, const SSkeleton* skeleton, SString& boneName)
    {
        if (!dynamic_cast<MSkeletalMeshEntity*>(target->getParent()))
            MInspectorField::statusText(EInspectorStatus::Error, "must be a direct child of a Skeletal Mesh entity");
        else if (!skeleton)
            MInspectorField::statusText(EInspectorStatus::Warning, "the parent has no skeletal mesh assigned");

        bool changed = false;
        START_FIELD_DRAW("Bone", "Saved by name, so it survives reimports.")
            changed = MBonePicker::draw("##bone", skeleton, boneName);
        END_FIELD_DRAW()

        if (skeleton && !boneName.empty() && skeleton->findBone(boneName) < 0)
            MInspectorField::statusText(EInspectorStatus::Error, "no bone of this name in the skeleton");
        return changed;
    }

    bool drawQuaternionAsEuler(const char* label, SQuaternion& q)
    {
        SVector3 euler = quaternionToEuler(q);   // degrees
        bool changed = false;
        START_FIELD_DRAW(label)
            if (ImGui::DragFloat3("##v", &euler.x, 0.5f, 0.0f, 0.0f, "%.1f"))
            {
                q = eulerToQuaternion(euler);            // degrees
                changed = true;
            }
        END_FIELD_DRAW()
        return changed;
    }

    bool drawVector(const char* label, SVector3& v, float speed = 0.01f)
    {
        bool changed = false;
        START_FIELD_DRAW(label)
            changed = ImGui::DragFloat3("##v", &v.x, speed, 0.0f, 0.0f, "%.3f");
        END_FIELD_DRAW()
        return changed;
    }

    bool drawCheckbox(const char* label, Field<bool>& field, const char* tooltip = nullptr)
    {
        bool v = field.get();
        bool changed = false;
        START_FIELD_DRAW(label, tooltip)
            if (ImGui::Checkbox("##v", &v)) { field = v; changed = true; }
        END_FIELD_DRAW()
        return changed;
    }
}

// -- Socket ----------------------------------------------------------------------------

bool MBoneSocketInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MBoneSocketEntity*>(entity) != nullptr;
}

void MBoneSocketInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    auto* socket = dynamic_cast<MBoneSocketEntity*>(target);
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    if (!socket || !ImGui::CollapsingHeader("Bone Socket", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    auto* mesh = socket->getSkeletalMeshParent();
    SString bone = socket->getBoneName();
    if (drawBoneRow(socket, mesh ? mesh->getSkeleton() : nullptr, bone))
        socket->setBoneName(bone);

    drawCheckbox("Follow Position", socket->followPosition);
    drawCheckbox("Follow Rotation", socket->followRotation);
    drawCheckbox("Follow Scale",    socket->followScale);

    MInspectorField::headerRow("Offset (applied after the bone)");
    SVector3 position = socket->offsetPosition.get();
    if (drawVector("Position", position)) socket->offsetPosition = position;
    SQuaternion rotation = socket->offsetRotation.get();
    if (drawQuaternionAsEuler("Rotation", rotation)) socket->offsetRotation = rotation;
    SVector3 scale = socket->offsetScale.get();
    if (drawVector("Scale", scale)) socket->offsetScale = scale;

    drawCheckbox("Keep Updated Offscreen", socket->keepUpdatedOffscreen,
                 "Keeps evaluating bones (not skinning) while the character is culled - "
                 "for hitboxes, muzzle points and other gameplay attachments.");
}

// -- Control ---------------------------------------------------------------------------

bool MBoneControlInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MBoneControlEntity*>(entity) != nullptr;
}

void MBoneControlInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    auto* control = dynamic_cast<MBoneControlEntity*>(target);
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    if (!control || !ImGui::CollapsingHeader("Bone Control", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    auto* mesh = control->getSkeletalMeshParent();
    SString bone = control->getBoneName();
    if (drawBoneRow(control, mesh ? mesh->getSkeleton() : nullptr, bone))
    {
        control->setBoneName(bone);
        // Choosing a bone snaps the control to the bone's bind-pose transform.
        control->snapToBone();
    }

    // Two controls on the same bone: the last in hierarchy order wins.
    if (mesh && !bone.empty())
    {
        bool seenSelf = false, laterSibling = false, earlierSibling = false;
        for (auto* sibling : mesh->getChildren())
        {
            if (sibling == control) { seenSelf = true; continue; }
            auto* other = dynamic_cast<MBoneControlEntity*>(sibling);
            if (!other || other->getBoneName() != bone) continue;
            (seenSelf ? laterSibling : earlierSibling) = true;
        }
        if (laterSibling)
            MInspectorField::statusText(EInspectorStatus::Warning, "another control on this bone comes later and wins");
        else if (earlierSibling)
            MInspectorField::statusText(EInspectorStatus::Warning, "another control drives this bone - this one wins");
    }

    START_FIELD_DRAW("Setup")
        if (ImGui::Button("Snap To Bone", ImVec2(-FLT_MIN, 0)))
            control->snapToBone();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Move the control onto the bone's bind pose and reset the Additive start and axes");
    END_FIELD_DRAW()

    static const char* modeLabels[] = { "Override", "Additive" };
    START_FIELD_DRAW("Mode", "Override replaces the animated rotation.\n"
                             "Additive adds the control's change from its snapped start on top of it.")
        int mode = static_cast<int>(control->mode.get());
        if (ImGui::Combo("##v", &mode, modeLabels, IM_ARRAYSIZE(modeLabels)))
            control->mode = static_cast<EBoneControlMode>(mode);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Weight", "Blends between the animated rotation (0) and the control's (1)")
        float v = control->weight.get();
        if (MInspectorField::SliderFloat("##v", &v, 0.0f, 1.0f, "%.2f")) control->weight = v;
    END_FIELD_DRAW()

    START_FIELD_DRAW("Max Angle", "Degrees the bone may turn from its animated rotation. 0 = no limit.")
        float v = control->maxAngle.get();
        if (MInspectorField::DragFloat("##v", &v, 0.5f, 0.0f, 180.0f, "%.1f deg")) control->maxAngle = v;
    END_FIELD_DRAW()

    MInspectorField::headerRow("Look");
    SVector3 aim = control->aimAxis.get();
    if (drawVector("Aim Axis", aim) && glm::length(aim) > 1e-4f) control->aimAxis = glm::normalize(aim);
    SVector3 up = control->upAxis.get();
    if (drawVector("Up Axis", up) && glm::length(up) > 1e-4f) control->upAxis = glm::normalize(up);

    START_FIELD_DRAW("Turn Speed", "Degrees per second for look turns. 0 = instant.")
        float v = control->turnSpeed.get();
        if (MInspectorField::DragFloat("##v", &v, 1.0f, 0.0f, 3600.0f, "%.0f deg/s")) control->turnSpeed = v;
    END_FIELD_DRAW()

    START_FIELD_DRAW("Blend Time", "Seconds to fade in on setLookTarget and out on clearLookTarget")
        float v = control->blendTime.get();
        if (MInspectorField::DragFloat("##v", &v, 0.01f, 0.0f, 10.0f, "%.2f s")) control->blendTime = v;
    END_FIELD_DRAW()
}
