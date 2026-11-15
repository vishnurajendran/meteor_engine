#include <GL/glew.h>
#include "skeletal_mesh_entity.h"

#include <algorithm>
#include <cfloat>

#include "anim_clip.h"
#include "anim_clip_asset.h"
#include "bone_socket_entity.h"
#include "core/application/application.h"
#include "core/engine/animation/animation_helpers.h"
#include "core/engine/animation/animation_subsystem.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/scripting/scripting_call_symbols.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/interfaces/render_item_collector.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "skeletal_mesh.h"

IMPLEMENT_SPATIAL_CLASS(MSkeletalMeshEntity)

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

MSkeletalMeshEntity::MSkeletalMeshEntity()
{
    name = "SkeletalMeshEntity";
    if (auto* anim = MAnimationHelpers::getSubsystem())
        handle = anim->registerInstance(this);
}

MSkeletalMeshEntity::~MSkeletalMeshEntity()
{
    if (auto* anim = MAnimationHelpers::getSubsystem())
        anim->unregisterInstance(handle);
    releaseSkinnedOutputs();
}

void MSkeletalMeshEntity::onExit()
{
    MMeshEntityBase::onExit();
    if (auto* anim = MAnimationHelpers::getSubsystem())
        anim->unregisterInstance(handle);
}

void MSkeletalMeshEntity::onStart()
{
    MMeshEntityBase::onStart();

    if (auto* inst = instance())
        inst->player.speed = speed.get();

    const MApplication* app = MApplication::getAppInstance();
    if (playOnStart.get() && !defaultClip.get().empty() && app && app->isSimulating())
        playClip(defaultClip.get(), 0.0f, loop.get());
}

SAnimInstance* MSkeletalMeshEntity::instance()
{
    auto* anim = MAnimationHelpers::getSubsystem();
    return anim ? anim->getInstance(handle) : nullptr;
}

const SAnimInstance* MSkeletalMeshEntity::instance() const
{
    const auto* anim = MAnimationHelpers::getSubsystem();
    return anim ? anim->getInstance(handle) : nullptr;
}

// ---------------------------------------------------------------------------
// Serialization
//
//   <materialSlots> ... </materialSlots>            (see MMeshEntityBase)
//   <animationSources>
//       <source>guid:...</source>                    in order - later wins
//   </animationSources>
// ---------------------------------------------------------------------------

void MSkeletalMeshEntity::onSerialise(pugi::xml_node& node)
{
    MSpatialEntity::onSerialise(node);
    serialiseMaterialSlots(node);

    auto sourcesNode = node.append_child("animationSources");
    for (const auto& ref : animationSources)
    {
        ref.resolve();   // upgrade path-only refs to a GUID
        const SString text = !ref.getAssetId().empty()
            ? SString(ASSET_REF_GUID_PREFIX) + ref.getAssetId()
            : ref.getPath();
        sourcesNode.append_child("source").text().set(text.c_str());
    }
}

void MSkeletalMeshEntity::onDeserialise(const pugi::xml_node& node)
{
    MSpatialEntity::onDeserialise(node);

    if (auto* asset = meshAsset.get().resolve())
        resizeMaterialSlots(static_cast<int>(asset->getMeshes().size()));
    deserialiseMaterialSlots(node);

    animationSources.clear();
    for (auto sourceNode : node.child("animationSources").children("source"))
    {
        const SString ref(sourceNode.text().as_string(""));
        if (ref.empty()) continue;
        if (IAssetManagerSubsystem::isGuidReference(ref))
            animationSources.emplace_back(SString(ref.str().substr(ASSET_REF_GUID_PREFIX_LEN)));
        else
            animationSources.emplace_back(SString{}, ref);
        animationSources.back().resolve();
    }

    clipTableValid = false;
    hasModelBounds = false;
}

// ---------------------------------------------------------------------------
// Mesh and sources
// ---------------------------------------------------------------------------

void MSkeletalMeshEntity::setSkeletalMeshAsset(TAssetRef<MSkeletalMeshAsset> asset)
{
    meshAsset = asset;
    auto* resolved = meshAsset.get().resolve();
    resizeMaterialSlots(resolved ? static_cast<int>(resolved->getMeshes().size()) : 0);
    clipTableValid = false;
    hasModelBounds = false;
}

const SSkeleton* MSkeletalMeshEntity::getSkeleton() const
{
    const auto* asset = resolveMeshAsset();
    return asset && asset->isValid() ? &asset->getSkeleton() : nullptr;
}

bool MSkeletalMeshEntity::isAnimationSourceAsset(const MAsset* asset)
{
    return dynamic_cast<const IAnimationClipSource*>(asset) != nullptr;
}

void MSkeletalMeshEntity::addAnimationSource(const TAssetRef<MAsset>& source)
{
    animationSources.push_back(source);
    clipTableValid = false;
}

void MSkeletalMeshEntity::setAnimationSource(int index, const TAssetRef<MAsset>& source)
{
    if (index < 0 || index >= getAnimationSourceCount()) return;
    animationSources[index] = source;
    clipTableValid = false;
}

void MSkeletalMeshEntity::removeAnimationSource(int index)
{
    if (index < 0 || index >= getAnimationSourceCount()) return;
    animationSources.erase(animationSources.begin() + index);
    clipTableValid = false;
}

void MSkeletalMeshEntity::moveAnimationSource(int from, int to)
{
    const int count = getAnimationSourceCount();
    if (from < 0 || from >= count || to < 0 || to >= count || from == to) return;
    auto moved = animationSources[from];
    animationSources.erase(animationSources.begin() + from);
    animationSources.insert(animationSources.begin() + to, moved);
    clipTableValid = false;
}

// ---------------------------------------------------------------------------
// Clip table
// ---------------------------------------------------------------------------

uint64_t MSkeletalMeshEntity::computeClipSignature() const
{
    uint64_t sig = MAnimationHelpers::HASH_SEED;
    const auto* mesh = resolveMeshAsset();
    sig = MAnimationHelpers::hashCombine(sig, reinterpret_cast<uintptr_t>(mesh));
    sig = MAnimationHelpers::hashCombine(sig, mesh ? mesh->getGeneration() : 0);
    for (const auto& ref : animationSources)
    {
        const MAsset* asset = ref.resolve();
        const auto* source  = dynamic_cast<const IAnimationClipSource*>(asset);
        sig = MAnimationHelpers::hashCombine(sig, reinterpret_cast<uintptr_t>(asset));
        sig = MAnimationHelpers::hashCombine(sig, source ? source->getClipGeneration() : 0);
    }
    return MAnimationHelpers::hashCombine(sig, animationSources.size());
}

void MSkeletalMeshEntity::ensureClipTable() const
{
    const uint64_t signature = computeClipSignature();
    if (clipTableValid && signature == clipTableSignature)
        return;
    rebuildClipTable();
    clipTableSignature = signature;
    clipTableValid     = true;
}

void MSkeletalMeshEntity::rebuildClipTable() const
{
    clipTable.clear();

    const MSkeletalMeshAsset* mesh = resolveMeshAsset();
    const SSkeleton* skeleton = mesh && mesh->isValid() ? &mesh->getSkeleton() : nullptr;
    const SString skeletonName = mesh ? FileIO::getFileNameWithoutExtension(mesh->getPath()) : SString();

    std::vector<const MAsset*> seen;
    auto addSource = [&](const MAsset* asset, bool fromMesh)
    {
        const auto* source = dynamic_cast<const IAnimationClipSource*>(asset);
        if (!asset || !source)
            return;
        if (std::find(seen.begin(), seen.end(), asset) != seen.end())
            return;   // duplicate source, or the mesh's own asset added as one
        seen.push_back(asset);

        for (MAnimClip* clip : source->getClips())
        {
            if (!clip) continue;
            SClipTableEntry entry;
            entry.clip       = clip;
            entry.name       = clip->getName();
            entry.sourcePath = asset->getPath();
            entry.fromMesh   = fromMesh;
            entry.duration   = clip->getDuration();

            if (skeleton)
            {
                const SClipBinding& binding = MBoneBinding::getBinding(*clip, *skeleton);
                entry.binding      = &binding;
                entry.status       = binding.status;
                entry.matchedBones = binding.matchedBones;
                entry.totalBones   = binding.totalBones;
                entry.reason       = MBoneBinding::describeUnavailable(binding, *skeleton, skeletonName);
            }
            else
            {
                entry.status = EClipStatus::Unavailable;
                entry.reason = "no skeletal mesh";
            }

            // A later clip with the same name overrides the earlier one.
            for (auto& earlier : clipTable)
            {
                if (earlier.name == entry.name && earlier.status != EClipStatus::Overridden)
                {
                    earlier.status       = EClipStatus::Overridden;
                    earlier.overriddenBy = entry.sourcePath;
                }
            }
            clipTable.push_back(std::move(entry));
        }
    };

    if (mesh)
        addSource(mesh, true);
    for (const auto& ref : animationSources)
        addSource(ref.resolve(), false);
}

const std::vector<SClipTableEntry>& MSkeletalMeshEntity::getClipTable() const
{
    ensureClipTable();
    return clipTable;
}

const SClipTableEntry* MSkeletalMeshEntity::findPlayableClip(const SString& clipName) const
{
    for (auto it = clipTable.rbegin(); it != clipTable.rend(); ++it)
        if (it->name == clipName && it->isPlayable())
            return &*it;
    return nullptr;
}

void MSkeletalMeshEntity::refreshPlayerClips(SAnimationPlayer& player) const
{
    // Clip pointers may belong to an asset that reloaded since last frame.
    player.removeEntriesIf([this](SAnimationPlayerEntry& entry)
    {
        const SClipTableEntry* clip = findPlayableClip(entry.clipName);
        if (!clip)
        {
            entry.clip    = nullptr;
            entry.binding = nullptr;
            return true;   // the clip is gone or no longer playable
        }
        entry.clip     = clip->clip;
        entry.binding  = clip->binding;
        entry.duration = clip->duration;
        entry.time     = std::min(entry.time, entry.duration);
        return false;
    });
}

// ---------------------------------------------------------------------------
// Playback
// ---------------------------------------------------------------------------

bool MSkeletalMeshEntity::playClip(const SString& clipName, float blendTime, bool loopClip)
{
    ensureClipTable();
    SAnimInstance* inst = instance();
    if (!inst)
        return false;

    const SClipTableEntry* entry = findPlayableClip(clipName);
    if (!entry)
    {
        // Same reason the inspector shows, e.g. "Dance (Dance.fbx): no bones match Hero skeleton".
        SString reason;
        for (auto it = clipTable.rbegin(); it != clipTable.rend() && reason.empty(); ++it)
        {
            if (it->name != clipName) continue;
            reason = SString::format("{0} ({1}): {2}", clipName, MAnimationHelpers::displayName(it->sourcePath),
                                     it->status == EClipStatus::Overridden
                                         ? SString("overridden by ") + MAnimationHelpers::displayName(it->overriddenBy)
                                         : it->reason);
        }
        if (reason.empty())
            reason = SString::format("no clip named '{0}' on '{1}'", clipName, getName());
        MWARN(STR("playClip: ") + reason);
        return false;
    }

    inst->player.play(clipName, entry->clip, entry->binding, entry->duration, std::max(0.0f, blendTime), loopClip);
    return true;
}

void MSkeletalMeshEntity::stop(float blendTime)
{
    if (auto* inst = instance())
        inst->player.stop(std::max(0.0f, blendTime));
}

void MSkeletalMeshEntity::setSpeed(float newSpeed)
{
    speed = newSpeed;
    if (auto* inst = instance())
        inst->player.speed = newSpeed;
}

bool MSkeletalMeshEntity::isPlaying() const
{
    const auto* inst = instance();
    return inst && inst->player.isPlaying();
}

SString MSkeletalMeshEntity::getCurrentClip() const
{
    const auto* inst = instance();
    if (!inst) return {};
    const int i = inst->player.dominantEntry();
    return i >= 0 ? inst->player.entries[i].clipName : SString();
}

float MSkeletalMeshEntity::getNormalizedTime() const
{
    const auto* inst = instance();
    if (!inst) return 0.0f;
    const int i = inst->player.dominantEntry();
    if (i < 0) return 0.0f;
    const auto& e = inst->player.entries[i];
    return e.duration > 0.0f ? e.time / e.duration : 0.0f;
}

void MSkeletalMeshEntity::setNormalizedTime(float t)
{
    auto* inst = instance();
    if (!inst) return;
    const int i = inst->player.dominantEntry();
    if (i < 0) return;
    auto& e = inst->player.entries[i];
    e.time = std::clamp(t, 0.0f, 1.0f) * e.duration;
    if (e.time < e.duration)
        e.finished = false;
}

bool MSkeletalMeshEntity::hasClip(const SString& clipName) const
{
    ensureClipTable();
    return findPlayableClip(clipName) != nullptr;
}

std::vector<SString> MSkeletalMeshEntity::getClipNames() const
{
    ensureClipTable();
    std::vector<SString> names;
    for (const auto& entry : clipTable)
        if (entry.isPlayable() && std::find(names.begin(), names.end(), entry.name) == names.end())
            names.push_back(entry.name);
    return names;
}

// ---------------------------------------------------------------------------
// Bones
// ---------------------------------------------------------------------------

int MSkeletalMeshEntity::findBoneIndex(const SString& boneName) const
{
    const SSkeleton* skeleton = getSkeleton();
    return skeleton ? skeleton->findBone(boneName) : -1;
}

int MSkeletalMeshEntity::getBoneCount() const
{
    const SSkeleton* skeleton = getSkeleton();
    return skeleton ? skeleton->getBoneCount() : 0;
}

SString MSkeletalMeshEntity::getBoneName(int bone) const
{
    const SSkeleton* skeleton = getSkeleton();
    return skeleton && bone >= 0 && bone < skeleton->getBoneCount() ? skeleton->bones[bone].name : SString();
}

SMatrix4 MSkeletalMeshEntity::getBoneModelTransform(int bone) const
{
    const auto* inst = instance();
    if (inst && inst->hasPose && bone >= 0 && bone < static_cast<int>(inst->model.size()))
        return inst->model[bone];

    // Not evaluated yet: the bind pose.
    const SSkeleton* skeleton = getSkeleton();
    if (!skeleton || bone < 0 || bone >= skeleton->getBoneCount())
        return SMatrix4(1.0f);
    std::vector<SMatrix4> bind;
    skeleton->computeBindModelTransforms(bind);
    return bind[bone];
}

SVector3 MSkeletalMeshEntity::getBoneModelPosition(int bone) const
{
    return SVector3(getBoneModelTransform(bone)[3]);
}

SQuaternion MSkeletalMeshEntity::getBoneModelRotation(int bone) const
{
    return STransform::fromMatrix(getBoneModelTransform(bone)).rotation;
}

SMatrix4 MSkeletalMeshEntity::getBoneWorldTransform(const SString& boneName) const
{
    const int bone = findBoneIndex(boneName);
    if (bone < 0)
        return SMatrix4(1.0f);
    return getTransformMatrix() * getBoneModelTransform(bone);
}

SVector3 MSkeletalMeshEntity::getBoneWorldPosition(const SString& boneName) const
{
    return SVector3(getBoneWorldTransform(boneName)[3]);
}

SQuaternion MSkeletalMeshEntity::getBoneWorldRotation(const SString& boneName) const
{
    return STransform::fromMatrix(getBoneWorldTransform(boneName)).rotation;
}

STransform MSkeletalMeshEntity::getBoneLocalTransform(int bone) const
{
    const auto* inst = instance();
    if (inst && inst->hasPose && bone >= 0 && bone < static_cast<int>(inst->local.size()))
        return inst->local[bone];
    const SSkeleton* skeleton = getSkeleton();
    if (skeleton && bone >= 0 && bone < skeleton->getBoneCount())
        return skeleton->bones[bone].bindPose;
    return {};
}

void MSkeletalMeshEntity::warnPoseWriteOutsideHook() const
{
    if (warnedPoseWrite) return;
    warnedPoseWrite = true;
    MWARN(SString::format("'{0}': setBoneLocalTransform outside onPostAnimate is ignored - "
                          "the next evaluation would overwrite it", getName()));
}

void MSkeletalMeshEntity::setBoneLocalTransform(int bone, const STransform& transform)
{
    SAnimInstance* inst = instance();
    if (!inst || !inst->poseEditable || !inst->skeleton)
    {
        warnPoseWriteOutsideHook();
        return;
    }
    if (bone < 0 || bone >= static_cast<int>(inst->local.size()))
        return;

    const SMatrix4 world = getTransformMatrix();
    SPoseEditContext context(*inst, *inst->skeleton, world);
    context.setLocal(bone, transform);   // re-walks the subtree; skinning matrices follow in step 4
}

// ---------------------------------------------------------------------------
// GPU outputs
// ---------------------------------------------------------------------------

void MSkeletalMeshEntity::ensureSkinnedOutputs(MSkeletalMeshAsset* asset)
{
    const auto& meshes = asset->getMeshes();
    if (asset == outputsAsset && asset->getGeneration() == outputsGeneration && skinnedOutputs.size() == meshes.size())
        return;

    releaseSkinnedOutputs();
    // The asset (re)loaded: its sub-mesh count may have changed.
    resizeMaterialSlots(static_cast<int>(meshes.size()));
    hasModelBounds = false;

    skinnedOutputs.resize(meshes.size());
    for (size_t i = 0; i < meshes.size(); ++i)
    {
        const MSkeletalMesh* mesh = meshes[i];
        SSkinnedOutput& out = skinnedOutputs[i];
        if (!mesh || mesh->getVertexCount() == 0) continue;

        const auto bytes = static_cast<GLsizeiptr>(mesh->getVertexCount() * sizeof(SVertex));
        out.vertexCount = mesh->getVertexCount();

        glGenBuffers(1, &out.buffer);
        glBindBuffer(GL_ARRAY_BUFFER, out.buffer);
        glBufferData(GL_ARRAY_BUFFER, bytes, nullptr, GL_DYNAMIC_COPY);

        // Start as the bind pose: if the character is never skinned (culled
        // from the first frame), shadows still draw something sensible.
        glBindBuffer(GL_COPY_READ_BUFFER, mesh->getBindPoseBuffer());
        glBindBuffer(GL_COPY_WRITE_BUFFER, out.buffer);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, bytes);
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

        // A normal VAO over the output - every stage draws it unchanged.
        glGenVertexArrays(1, &out.vao);
        glBindVertexArray(out.vao);
        glBindBuffer(GL_ARRAY_BUFFER, out.buffer);
        setupSVertexAttributes();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->getEBO());
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    outputsAsset      = asset;
    outputsGeneration = asset->getGeneration();
}

void MSkeletalMeshEntity::releaseSkinnedOutputs()
{
    const bool canDelete = isGLContextCurrent();
    for (auto& out : skinnedOutputs)
    {
        if (!canDelete) break;
        if (out.vao)    glDeleteVertexArrays(1, &out.vao);
        if (out.buffer) glDeleteBuffers(1, &out.buffer);
    }
    skinnedOutputs.clear();
    outputsAsset      = nullptr;
    outputsGeneration = 0;
}

// ---------------------------------------------------------------------------
// Bounds
// ---------------------------------------------------------------------------

void MSkeletalMeshEntity::updateWorldBoundsFromModel(const SVector3& modelMin, const SVector3& modelMax)
{
    const SMatrix4 world = getTransformMatrix();
    SVector3 mn(FLT_MAX), mx(-FLT_MAX);
    for (int c = 0; c < 8; ++c)
    {
        const SVector3 corner((c & 1) ? modelMax.x : modelMin.x,
                              (c & 2) ? modelMax.y : modelMin.y,
                              (c & 4) ? modelMax.z : modelMin.z);
        const SVector3 w = SVector3(world * SVector4(corner, 1.0f));
        mn = glm::min(mn, w);
        mx = glm::max(mx, w);
    }
    bounds.min = mn;
    bounds.max = mx;
}

AABB MSkeletalMeshEntity::animGetCullBounds() const
{
    return bounds;
}

void MSkeletalMeshEntity::animOnPoseUpdated(const SAnimInstance& inst)
{
    const auto* asset = resolveMeshAsset();
    const SVector3 padding = asset ? asset->getJointBoundsPadding() : SVector3(0.0f);
    lastModelMin   = inst.jointMin - padding;
    lastModelMax   = inst.jointMax + padding;
    hasModelBounds = true;
    updateWorldBoundsFromModel(lastModelMin, lastModelMax);
}

// ---------------------------------------------------------------------------
// Subsystem hooks
// ---------------------------------------------------------------------------

const SSkeleton* MSkeletalMeshEntity::animPrepareFrame(SAnimInstance& inst)
{
    MSkeletalMeshAsset* asset = resolveMeshAsset();
    if (!asset || !asset->isValid() || asset->getMeshes().empty())
        return nullptr;

    ensureSkinnedOutputs(asset);
    ensureClipTable();
    refreshPlayerClips(inst.player);

    // World bounds for this frame's culling test, from the last pose (the
    // entity may have moved since). While culled the pose stops updating, so
    // pad generously - otherwise a character can stay culled after it should
    // have come back into view.
    if (hasModelBounds)
    {
        SVector3 mn = lastModelMin, mx = lastModelMax;
        if (!inst.visible)
        {
            const SVector3 grow = (mx - mn) * 0.25f;
            mn -= grow;
            mx += grow;
        }
        updateWorldBoundsFromModel(mn, mx);
    }
    else
    {
        const SVector3 pad = asset->getJointBoundsPadding();
        updateWorldBoundsFromModel(asset->getBoundsMin() - pad, asset->getBoundsMax() + pad);
    }

    return &asset->getSkeleton();
}

bool MSkeletalMeshEntity::animHasOffscreenSocket() const
{
    for (auto* child : children)
        if (auto* socket = dynamic_cast<MBoneSocketEntity*>(child))
            if (socket->isEnabledInHierarchy() && socket->keepUpdatedOffscreen.get())
                return true;
    return false;
}

void MSkeletalMeshEntity::animCollectSkinJobs(uint32_t boneOffset, std::vector<SSkinningJob>& jobs) const
{
    if (!outputsAsset) return;
    const auto& meshes = outputsAsset->getMeshes();
    for (size_t i = 0; i < skinnedOutputs.size() && i < meshes.size(); ++i)
    {
        if (!skinnedOutputs[i].buffer) continue;
        jobs.push_back({ meshes[i], skinnedOutputs[i].buffer, boneOffset });
    }
}

void MSkeletalMeshEntity::animDispatchClipFinished(const SString& clipName)
{
    onClipFinished.broadcast(clipName);

    IScriptInstance* script = getScriptInstance();
    if (!script || !script->isValid() || !hasStarted() || hasExited())
        return;
    if (!script->hasFunc(ScriptinCallSymbols::SCRIPT_FUNC_CLIP_FINISHED))
        return;
    SDynValue arg, result;
    arg.setString(clipName.str());
    script->callFunc(ScriptinCallSymbols::SCRIPT_FUNC_CLIP_FINISHED, std::vector<SDynValue>{ arg }, result);
}

void MSkeletalMeshEntity::animRunPostAnimate(float deltaTime)
{
    onPostAnimate(deltaTime);

    // Whether the script defines onPostAnimate is checked once per script
    // instance, not every frame.
    IScriptInstance* script = getScriptInstance();
    if (script != cachedScriptInstance)
    {
        cachedScriptInstance = script;
        scriptHasPostAnimate = script && script->isValid()
                            && script->hasFunc(ScriptinCallSymbols::SCRIPT_FUNC_POST_ANIMATE);
    }
    if (!scriptHasPostAnimate || !script || !hasStarted() || hasExited())
        return;

    SDynValue dt, result;
    dt.setFloat(deltaTime);
    script->callFunc(ScriptinCallSymbols::SCRIPT_FUNC_POST_ANIMATE, std::vector<SDynValue>{ dt }, result);
}

void MSkeletalMeshEntity::animUpdateSockets(const SAnimInstance& inst)
{
    if (!inst.skeleton) return;
    for (auto* child : children)
        if (auto* socket = dynamic_cast<MBoneSocketEntity*>(child))
            if (socket->isEnabledInHierarchy())
                socket->applyBonePose(*inst.skeleton, inst.model);
}

#if METEOR_EDITOR
void MSkeletalMeshEntity::animDriveEditorPreview(SAnimInstance& inst, float deltaTime)
{
    SAnimationPlayer& player = inst.player;
    if (previewClip.empty())
    {
        player.clear();   // bind pose
        return;
    }

    const bool current = player.entryCount > 0 && player.entries[player.entryCount - 1].clipName == previewClip;
    if (!current)
    {
        const SClipTableEntry* entry = findPlayableClip(previewClip);
        player.clear();
        if (!entry)
            return;
        player.play(previewClip, entry->clip, entry->binding, entry->duration, 0.0f, true);
    }

    player.speed = speed.get();
    auto& e = player.entries[player.entryCount - 1];
    if (previewSeek >= 0.0f)
    {
        e.time = std::clamp(previewSeek, 0.0f, 1.0f) * e.duration;
        previewSeek = -1.0f;
    }
    if (previewPlaying)
    {
        std::vector<SString> ignored;
        player.advance(deltaTime, ignored);
    }
}
#endif

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void MSkeletalMeshEntity::submitRenderItem(IRenderItemCollector* collector)
{
    if (!isEnabledInHierarchy())
        return;

    MSkeletalMeshAsset* asset = resolveMeshAsset();
    if (!asset || !asset->isValid())
        return;

    const SMatrix4 world  = getTransformMatrix();
    const auto&    meshes = asset->getMeshes();
    const bool outputsCurrent = asset == outputsAsset && asset->getGeneration() == outputsGeneration;
    const int  drawCount = std::min(static_cast<int>(meshes.size()), static_cast<int>(materialSlots.size()));

    for (int i = 0; i < drawCount; ++i)
    {
        const MSkeletalMesh* mesh = meshes[i];
        if (!mesh) continue;

        MMaterial* mat = resolveSlotMaterialOrError(i);
        if (!mat) continue;

        // The skinned output, behind an ordinary VAO; the bind pose until
        // the subsystem has made one.
        const bool skinned = outputsCurrent && i < static_cast<int>(skinnedOutputs.size()) && skinnedOutputs[i].vao;

        SRenderItem item;
        item.vao         = skinned ? skinnedOutputs[i].vao : mesh->getBindVAO();
        item.ebo         = mesh->getEBO();
        item.indexCount  = mesh->getIndexCount();
        item.vertexCount = mesh->getVertexCount();
        item.transform   = world;
        fillCommonRenderItem(item, mat);
        collector->submitRenderItem(item);
    }
}

// ---------------------------------------------------------------------------
// Gizmos
// ---------------------------------------------------------------------------

void MSkeletalMeshEntity::onDrawGizmo(SVector2 res)
{
    MMeshEntityBase::onDrawGizmo(res);

#if METEOR_EDITOR
    if (!debug_drawSkeleton)
        return;
    const SSkeleton* skeleton = getSkeleton();
    if (!skeleton)
        return;

    bool anyDeform = false;
    for (const auto& bone : skeleton->bones) anyDeform |= bone.isDeformBone;

    const SMatrix4 world = getTransformMatrix();
    for (int i = 0; i < skeleton->getBoneCount(); ++i)
    {
        const int parentIndex = skeleton->bones[i].parent;
        if (parentIndex < 0) continue;
        if (anyDeform && !skeleton->bones[i].isDeformBone) continue;
        const SVector3 a = SVector3(world * getBoneModelTransform(parentIndex)[3]);
        const SVector3 b = SVector3(world * getBoneModelTransform(i)[3]);
        MGizmos::drawLine(a, b, SColor::skyBlue(), 1.0f, true);
    }
#endif
}
