//
// skeletal_mesh_entity.h
//
// MSkeletalMeshEntity - a skinned character. Mirrors MStaticMeshEntity (it
// shares the material-slot code through MMeshEntityBase) and adds a list of
// animation sources and playback.
//
// Clip table: built on the main thread whenever the mesh or the sources
// change (asset reloads included) from
//   1. the mesh asset's own clips,
//   2. each source's clips in list order - a later clip with the same name
//      overrides the earlier one,
// skipping duplicate sources, including the mesh's own asset added as one.
// Each clip gets a status (Available / Warning / Unavailable / Overridden,
// see bone_binding.h). Embedded clips are never saved: they are rebuilt from
// the mesh asset whenever it resolves, so a re-exported file's new clip shows
// up without editing the scene.
//
// Runtime pose data lives in MAnimationSubsystem; this entity holds a handle.
//
#pragma once
#ifndef SKELETAL_MESH_ENTITY_H
#define SKELETAL_MESH_ENTITY_H

#include <vector>

#include "core/engine/3d/common/mesh_entity_base.h"
#include "core/engine/animation/bone_binding.h"
#include "core/engine/animation/pose.h"
#include "core/engine/animation/skinning_backend.h"
#include "core/utils/delegate.h"
#include "core/utils/transform.h"
#include "skeletal_mesh_asset.h"

class MAnimClip;
class MAnimationSubsystem;
class IScriptInstance;

// What a skeletal mesh does while it is outside every camera's view.
SCRIPT_BIND_ENUM()
enum class EAnimationCullingMode : int
{
    AdvanceTimeOnly = 0,   // clip time and onClipFinished keep going; no bones, modifiers, onPostAnimate or skinning
    Pause           = 1,   // nothing updates; playback resumes where it stopped
    AlwaysAnimate   = 2,   // full evaluation and skinning, as if visible
};

struct SClipTableEntry
{
    MAnimClip*          clip       = nullptr;
    const SClipBinding* binding    = nullptr;
    SString             name;
    SString             sourcePath;            // asset the clip comes from
    bool                fromMesh   = false;    // embedded in the mesh asset
    float               duration   = 0.0f;
    EClipStatus         status     = EClipStatus::Unavailable;
    int                 matchedBones = 0;
    int                 totalBones   = 0;
    SString             reason;                // why Unavailable
    SString             overriddenBy;          // source path that won, when Overridden

    [[nodiscard]] bool isPlayable() const { return status == EClipStatus::Available || status == EClipStatus::Warning; }
};

SCRIPT_BIND_CLASS()
class MSkeletalMeshEntity : public MMeshEntityBase
{
    DEFINE_SPATIAL_CLASS(MSkeletalMeshEntity)
    friend class MAnimationSubsystem;

    DECLARE_FIELD(meshAsset,   TAssetRef<MSkeletalMeshAsset>, {})
    DECLARE_FIELD(defaultClip, SString, "")      // kept even if the clip disappears (the inspector shows it red)
    DECLARE_FIELD(playOnStart, bool, true)
    DECLARE_FIELD(loop,        bool, true)
    DECLARE_FIELD(speed,       float, 1.0f)
    DECLARE_FIELD(cullingMode, EAnimationCullingMode, EAnimationCullingMode::AdvanceTimeOnly)

public:
    MSkeletalMeshEntity();
    ~MSkeletalMeshEntity() override;

    void submitRenderItem(IRenderItemCollector* collector) override;
    void onStart() override;
    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

    // -- Mesh -------------------------------------------------------------------------
    SCRIPT_BIND_FUNC()
    void setSkeletalMeshAsset(TAssetRef<MSkeletalMeshAsset> asset);
    SCRIPT_BIND_FUNC()
    TAssetHandle<MSkeletalMeshAsset> getSkeletalMeshAsset() const { return meshAsset.get().getHandle(); }
    [[nodiscard]] MSkeletalMeshAsset* resolveMeshAsset() const { return meshAsset.get().resolve(); }
    [[nodiscard]] const SSkeleton* getSkeleton() const;

    // -- Animation sources (MAnimClipAsset or MSkeletalMeshAsset) -------------------
    // Ordered: for clips with the same name, a later source wins.
    [[nodiscard]] int getAnimationSourceCount() const { return static_cast<int>(animationSources.size()); }
    [[nodiscard]] const TAssetRef<MAsset>& getAnimationSource(int index) const { return animationSources[index]; }
    void addAnimationSource(const TAssetRef<MAsset>& source);
    void setAnimationSource(int index, const TAssetRef<MAsset>& source);
    void removeAnimationSource(int index);
    void moveAnimationSource(int from, int to);
    // True if the asset can supply clips (implements IAnimationClipSource).
    static bool isAnimationSourceAsset(const MAsset* asset);

    // Every clip from the mesh and the sources, with its status - the
    // inspector's Available Clips table. Rebuilt here when anything changed.
    const std::vector<SClipTableEntry>& getClipTable() const;

    // -- Playback ---------------------------------------------------------------------
    // Crossfades to the clip over blendTime seconds. A no-op (returning true)
    // if it is already playing. Returns false and logs the reason for an
    // unknown or unavailable clip.
    SCRIPT_BIND_FUNC()
    bool playClip(const SString& clipName, float blendTime = 0.2f, bool loopClip = true);
    // Fades back to the bind pose.
    SCRIPT_BIND_FUNC()
    void stop(float blendTime = 0.2f);
    SCRIPT_BIND_FUNC()
    void setSpeed(float newSpeed);
    SCRIPT_BIND_FUNC()
    float getSpeed() const { return speed.get(); }
    SCRIPT_BIND_FUNC()
    bool isPlaying() const;
    // The clip with the highest weight; empty when nothing plays.
    SCRIPT_BIND_FUNC()
    SString getCurrentClip() const;
    SCRIPT_BIND_FUNC()
    float getNormalizedTime() const;
    SCRIPT_BIND_FUNC()
    void setNormalizedTime(float t);
    // Available clips only (Available or Warning status).
    SCRIPT_BIND_FUNC()
    bool hasClip(const SString& clipName) const;
    SCRIPT_BIND_FUNC()
    std::vector<SString> getClipNames() const;

    SCRIPT_BIND_FUNC()
    EAnimationCullingMode getCullingMode() const { return cullingMode.get(); }
    SCRIPT_BIND_FUNC()
    void setCullingMode(EAnimationCullingMode mode) { cullingMode = mode; }

    // Non-looping clips that reached their end. The entity's script also gets
    // onClipFinished(clipName), if it defines it.
    TMulticastDelegate<const SString&> onClipFinished;

    // -- Bones ------------------------------------------------------------------------
    // This frame's pose once the subsystem has run, so attachments belong in
    // lateUpdate. Identity for an unknown bone.
    SMatrix4 getBoneWorldTransform(const SString& boneName) const;
    SCRIPT_BIND_FUNC()
    SVector3 getBoneWorldPosition(const SString& boneName) const;
    SCRIPT_BIND_FUNC()
    SQuaternion getBoneWorldRotation(const SString& boneName) const;

    SCRIPT_BIND_FUNC()
    int getBoneCount() const;
    SCRIPT_BIND_FUNC()
    SString getBoneName(int bone) const;

    // -- Pose hook ----------------------------------------------------------------------
    // After the pose is computed and bone modifiers ran, before skinning - so
    // edits show on the mesh the same frame (look-at, aim offsets, simple foot
    // placement). Called only for instances evaluated this frame, in a fixed
    // order, on the main thread. Lua: function onPostAnimate(dt).
    virtual void onPostAnimate(float deltaTime) {}

    // Pose API - writes only take effect inside onPostAnimate (elsewhere they
    // are ignored with a warning: the next evaluation would overwrite them).
    // Look bone indices up once and cache them.
    SCRIPT_BIND_FUNC()
    int findBoneIndex(const SString& boneName) const;
    SCRIPT_BIND_FUNC()
    STransform getBoneLocalTransform(int bone) const;
    SCRIPT_BIND_FUNC()
    void setBoneLocalTransform(int bone, const STransform& transform);
    // Relative to this entity.
    SMatrix4 getBoneModelTransform(int bone) const;
    SCRIPT_BIND_FUNC()
    SVector3 getBoneModelPosition(int bone) const;
    SCRIPT_BIND_FUNC()
    SQuaternion getBoneModelRotation(int bone) const;

#if METEOR_EDITOR
    // -- Editor: clip preview outside simulation -----------------------------------
    void setPreviewClip(const SString& clipName) { previewClip = clipName; }
    [[nodiscard]] const SString& getPreviewClip() const { return previewClip; }
    void setPreviewPlaying(bool playing) { previewPlaying = playing; }
    [[nodiscard]] bool isPreviewPlaying() const { return previewPlaying; }
    // Scrubs the preview clip (0..1).
    void setPreviewNormalizedTime(float t) { previewSeek = t; }

    void setDrawSkeleton(bool draw) { debug_drawSkeleton = draw; }
    [[nodiscard]] bool getDrawSkeleton() const { return debug_drawSkeleton; }
#endif

protected:
    void onSerialise(pugi::xml_node& node) override;
    void onDeserialise(const pugi::xml_node& node) override;

private:
    struct SSkinnedOutput
    {
        unsigned int buffer      = 0;   // SVertex layout, written by skinning
        unsigned int vao         = 0;   // buffer + the mesh's EBO
        int          vertexCount = 0;
    };

    // -- Called by MAnimationSubsystem (main thread) --------------------------------
    // Resolves the mesh, rebuilds GPU outputs and the clip table if anything
    // changed, and refreshes the player's clip pointers. Null when there is
    // nothing to animate.
    const SSkeleton* animPrepareFrame(SAnimInstance& instance);
    AABB animGetCullBounds() const;
    bool animHasOffscreenSocket() const;
    void animOnPoseUpdated(const SAnimInstance& instance);
    void animCollectSkinJobs(uint32_t boneOffset, std::vector<SSkinningJob>& jobs) const;
    void animDispatchClipFinished(const SString& clipName);
    void animRunPostAnimate(float deltaTime);
    void animUpdateSockets(const SAnimInstance& instance);
#if METEOR_EDITOR
    // Drives the player from the preview settings while not simulating.
    void animDriveEditorPreview(SAnimInstance& instance, float deltaTime);
#endif

    // -- Internals ---------------------------------------------------------------------
    SAnimInstance*       instance();
    const SAnimInstance* instance() const;

    void ensureClipTable() const;
    void rebuildClipTable() const;
    uint64_t computeClipSignature() const;
    const SClipTableEntry* findPlayableClip(const SString& clipName) const;
    void refreshPlayerClips(SAnimationPlayer& player) const;

    void ensureSkinnedOutputs(MSkeletalMeshAsset* asset);
    void releaseSkinnedOutputs();

    void updateWorldBoundsFromModel(const SVector3& modelMin, const SVector3& modelMax);

    // Pose writes outside onPostAnimate warn once.
    void warnPoseWriteOutsideHook() const;

    SAnimInstanceHandle handle;

    std::vector<TAssetRef<MAsset>> animationSources;

    // Clip table cache - mutable: rebuilt lazily from const queries.
    mutable std::vector<SClipTableEntry> clipTable;
    mutable uint64_t                     clipTableSignature = 0;
    mutable bool                         clipTableValid     = false;

    // GPU outputs for the current mesh asset generation.
    std::vector<SSkinnedOutput> skinnedOutputs;
    uint64_t                    outputsGeneration = 0;
    const MSkeletalMeshAsset*   outputsAsset      = nullptr;

    // Model-space box of the last evaluated pose (joints + padding).
    SVector3 lastModelMin   = SVector3(0.0f);
    SVector3 lastModelMax   = SVector3(0.0f);
    bool     hasModelBounds = false;

    // onPostAnimate lookup, checked once per script instance.
    IScriptInstance* cachedScriptInstance = nullptr;
    bool             scriptHasPostAnimate = false;

    mutable bool warnedPoseWrite = false;

#if METEOR_EDITOR
    SString previewClip;
    bool    previewPlaying = false;
    float   previewSeek    = -1.0f;
    bool    debug_drawSkeleton = false;
#endif
};

#endif // SKELETAL_MESH_ENTITY_H
