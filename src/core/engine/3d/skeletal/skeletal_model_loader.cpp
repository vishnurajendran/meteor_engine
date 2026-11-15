#include "skeletal_model_loader.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "anim_clip.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/assetmanagement/source/assimp_asset_io.h"
#include "core/memory/mnew.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"

namespace
{
    // -- Conversions ----------------------------------------------------------------

    // aiMatrix4x4 is row-major (a1..a4 is the first row); glm takes columns.
    SMatrix4 toGlm(const aiMatrix4x4& m)
    {
        return SMatrix4(m.a1, m.b1, m.c1, m.d1,
                        m.a2, m.b2, m.c2, m.d2,
                        m.a3, m.b3, m.c3, m.d3,
                        m.a4, m.b4, m.c4, m.d4);
    }

    SVector3    toGlm(const aiVector3D& v)    { return SVector3(v.x, v.y, v.z); }
    SQuaternion toGlm(const aiQuaternion& q)  { return SQuaternion(q.w, q.x, q.y, q.z); }

    // S * m * S^-1 for a uniform scale S on an affine matrix: the linear part
    // is unchanged and the translation scales. That is what applying the
    // import scale to "bone translations and inverse-bind matrices" means.
    SMatrix4 scaleTranslation(SMatrix4 m, float s)
    {
        m[3] = SVector4(SVector3(m[3]) * s, m[3].w);
        return m;
    }

    bool matricesClose(const SMatrix4& a, const SMatrix4& b)
    {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                if (std::abs(a[c][r] - b[c][r]) > 1e-3f * std::max(1.0f, std::abs(a[c][r])))
                    return false;
        return true;
    }

    // -- Clip resampling --------------------------------------------------------------

    template <typename TKey>
    int findKey(const TKey* keys, unsigned int count, double ticks, int cursor)
    {
        // Keys are sorted; the cursor only moves forward as frames advance.
        while (cursor + 1 < static_cast<int>(count) && keys[cursor + 1].mTime <= ticks)
            ++cursor;
        return cursor;
    }

    SVector3 sampleVectorKeys(const aiVectorKey* keys, unsigned int count, double ticks, int& cursor)
    {
        if (count == 0) return SVector3(0.0f);
        if (count == 1 || ticks <= keys[0].mTime) return toGlm(keys[0].mValue);
        cursor = findKey(keys, count, ticks, cursor);
        if (cursor + 1 >= static_cast<int>(count)) return toGlm(keys[count - 1].mValue);
        const auto& a = keys[cursor];
        const auto& b = keys[cursor + 1];
        const double span = b.mTime - a.mTime;
        const float  t    = span > 0.0 ? static_cast<float>((ticks - a.mTime) / span) : 0.0f;
        return glm::mix(toGlm(a.mValue), toGlm(b.mValue), t);
    }

    SQuaternion sampleQuatKeys(const aiQuatKey* keys, unsigned int count, double ticks, int& cursor)
    {
        if (count == 0) return SQuaternion(1, 0, 0, 0);
        if (count == 1 || ticks <= keys[0].mTime) return glm::normalize(toGlm(keys[0].mValue));
        cursor = findKey(keys, count, ticks, cursor);
        if (cursor + 1 >= static_cast<int>(count)) return glm::normalize(toGlm(keys[count - 1].mValue));
        const auto& a = keys[cursor];
        const auto& b = keys[cursor + 1];
        const double span = b.mTime - a.mTime;
        const float  t    = span > 0.0 ? static_cast<float>((ticks - a.mTime) / span) : 0.0f;
        return glm::normalize(glm::slerp(glm::normalize(toGlm(a.mValue)), glm::normalize(toGlm(b.mValue)), t));
    }

    template <typename T, typename Eq>
    void collapseIfConstant(std::vector<T>& keys, Eq equal)
    {
        if (keys.size() <= 1) return;
        for (size_t i = 1; i < keys.size(); ++i)
            if (!equal(keys[0], keys[i]))
                return;
        keys.resize(1);
    }

    // -- Meshes -----------------------------------------------------------------------

    struct SMeshInstance
    {
        const aiMesh* mesh = nullptr;
        int           node = -1;
    };
}

// -- Importer setup -----------------------------------------------------------

void MSkeletalModelLoader::configureImporter(Assimp::Importer& importer)
{
    importer.SetIOHandler(new MAssimpAssetIOSystem(MAssetSources::getActive()));   // importer owns it
    // Without this, FBX pivots become extra "$AssimpFbx$" nodes between
    // bones, which clips from other files don't have tracks for.
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
}

// -- Skeleton -----------------------------------------------------------------

void MSkeletalModelLoader::collectNodes(const aiNode* node, int parent, std::vector<const aiNode*>& nodes, SSkeleton& skeleton)
{
    const int index = static_cast<int>(nodes.size());
    nodes.push_back(node);

    SBone bone;
    bone.name   = SString(node->mName.C_Str());
    bone.parent = parent;
    skeleton.bones.push_back(bone);

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        collectNodes(node->mChildren[i], index, nodes, skeleton);

    skeleton.bones[index].subtreeEnd = static_cast<int>(nodes.size());
}

// -- Clips --------------------------------------------------------------------

bool MSkeletalModelLoader::isGenericTakeName(const SString& name)
{
    const std::string& s = name.str();
    if (s.empty() || s == "mixamo.com" || s == "Take 001" || s == "Default Take")
        return true;
    const std::string mixamoSuffix = "|mixamo.com";
    return s.size() > mixamoSuffix.size()
        && s.compare(s.size() - mixamoSuffix.size(), mixamoSuffix.size(), mixamoSuffix) == 0;
}

void MSkeletalModelLoader::buildClips(const aiScene* scene, const SString& path, const SModelImportSettings& settings,
                                      std::vector<MAnimClip*>& outClips)
{
    const float scale = settings.scaleFactor;
    const float rate  = settings.sampleRate;
    std::unordered_set<SString> usedNames;

    for (unsigned int a = 0; a < scene->mNumAnimations; ++a)
    {
        const aiAnimation* anim = scene->mAnimations[a];
        const double tps        = anim->mTicksPerSecond > 0.0 ? anim->mTicksPerSecond : 25.0;
        const float  duration   = static_cast<float>(std::max(0.0, anim->mDuration / tps));
        const int    frameCount = duration > 0.0f
            ? std::max(2, static_cast<int>(std::ceil(duration * rate - 1e-4f)) + 1)
            : 1;

        // Mixamo and most DCC exports name a lone take "mixamo.com" or
        // "Take 001"; the file name says what it is far better.
        SString clipName(anim->mName.C_Str());
        if (scene->mNumAnimations == 1 && isGenericTakeName(clipName))
            clipName = FileIO::getFileNameWithoutExtension(path);
        else if (clipName.empty())
            clipName = SString("Clip ") + SString::fromInt(static_cast<int>(a));
        // Names must be unique within one source.
        if (usedNames.contains(clipName))
        {
            int n = 2;
            while (usedNames.contains(clipName + " (" + SString::fromInt(n) + ")")) ++n;
            clipName = clipName + " (" + SString::fromInt(n) + ")";
        }
        usedNames.insert(clipName);

        auto* clip = mnew<MAnimClip>();
        clip->setup(clipName, duration, rate, frameCount, path);
        auto& tracks = clip->editTracks();
        tracks.reserve(anim->mNumChannels);

        for (unsigned int c = 0; c < anim->mNumChannels; ++c)
        {
            const aiNodeAnim* channel = anim->mChannels[c];
            MAnimClip::STrack track;
            track.boneName = SString(channel->mNodeName.C_Str());
            track.positions.resize(frameCount);
            track.rotations.resize(frameCount);
            track.scales.resize(frameCount);

            int posCursor = 0, rotCursor = 0, sclCursor = 0;
            for (int f = 0; f < frameCount; ++f)
            {
                const double seconds = std::min(static_cast<double>(f) / rate, static_cast<double>(duration));
                const double ticks   = seconds * tps;
                track.positions[f] = sampleVectorKeys(channel->mPositionKeys, channel->mNumPositionKeys, ticks, posCursor) * scale;
                track.rotations[f] = sampleQuatKeys(channel->mRotationKeys, channel->mNumRotationKeys, ticks, rotCursor);
                track.scales[f]    = channel->mNumScalingKeys
                    ? sampleVectorKeys(channel->mScalingKeys, channel->mNumScalingKeys, ticks, sclCursor)
                    : SVector3(1.0f);

                // Keep neighbouring keys on one hemisphere, so the
                // runtime's two-key blend never takes the long way round.
                if (f > 0 && glm::dot(track.rotations[f - 1], track.rotations[f]) < 0.0f)
                    track.rotations[f] = -track.rotations[f];
            }

            collapseIfConstant(track.positions, [](const SVector3& x, const SVector3& y)
                               { return glm::all(glm::lessThan(glm::abs(x - y), SVector3(1e-6f))); });
            collapseIfConstant(track.rotations, [](const SQuaternion& x, const SQuaternion& y)
                               { return std::abs(glm::dot(x, y)) > 1.0f - 1e-7f; });
            collapseIfConstant(track.scales, [](const SVector3& x, const SVector3& y)
                               { return glm::all(glm::lessThan(glm::abs(x - y), SVector3(1e-6f))); });

            tracks.push_back(std::move(track));
        }

        outClips.push_back(clip);
    }
}

// -- Meshes -------------------------------------------------------------------

bool MSkeletalModelLoader::buildSkeletonAndMeshes(const aiScene* scene, const SString& path, const SModelImportSettings& settings,
                                                  SSkeletalModelData& out, SString& outError)
{
    const float scale = settings.scaleFactor;
    SSkeleton& skeleton = out.skeleton;

    // Every node of the file becomes a bone. Most are real bones; the rest
    // (the armature, mesh nodes, helpers) are cheap, keep the hierarchy's
    // transforms intact and give rigid parts something to follow.
    std::vector<const aiNode*> nodes;
    collectNodes(scene->mRootNode, -1, nodes, skeleton);
    skeleton.rebuildLookup();

    const int boneCount = skeleton.getBoneCount();
    std::vector<SMatrix4> globalBind(boneCount);   // scaled, without rootOffset
    for (int i = 0; i < boneCount; ++i)
    {
        SBone& bone   = skeleton.bones[i];
        bone.bindPose = STransform::fromMatrix(scaleTranslation(toGlm(nodes[i]->mTransformation), scale));
        const SMatrix4 local = bone.bindPose.toMatrix();
        globalBind[i] = bone.parent < 0 ? local : globalBind[bone.parent] * local;
        bone.inverseBind = glm::inverse(globalBind[i]);   // replaced below for deform bones
    }

    // Mesh instances in node order - the same order the static importer
    // uses, so material slot N means the same sub-mesh either way.
    std::vector<SMeshInstance> instances;
    for (int i = 0; i < boneCount; ++i)
        for (unsigned int m = 0; m < nodes[i]->mNumMeshes; ++m)
            instances.push_back({ scene->mMeshes[nodes[i]->mMeshes[m]], i });

    std::vector<bool> inverseBindSet(boneCount, false);
    bool warnedInverseBindMismatch = false;
    const int maxInfluences = settings.maxBoneInfluences;

    SVector3 meshMin(std::numeric_limits<float>::max());
    SVector3 meshMax(std::numeric_limits<float>::lowest());

    for (const auto& inst : instances)
    {
        const aiMesh*  mesh       = inst.mesh;
        const SMatrix4 meshGlobal = globalBind[inst.node];
        const SMatrix3 normalMat  = glm::transpose(glm::inverse(SMatrix3(meshGlobal)));
        const SMatrix4 invMeshGlobal = glm::inverse(meshGlobal);

        SSkeletalModelData::SSubMesh sub;
        const unsigned int vertexCount = mesh->mNumVertices;
        sub.vertices.resize(vertexCount);

        // Vertices go to the model's root space (the mesh node's bind
        // transform applied); the per-bone inverse bind below is made
        // relative to the same space, so skinning gives exactly what
        // Assimp's offset matrices describe.
        const bool hasUV = mesh->mTextureCoords[0] != nullptr;
        for (unsigned int v = 0; v < vertexCount; ++v)
        {
            SVertex& out_v = sub.vertices[v];
            out_v.Position = SVector3(meshGlobal * SVector4(toGlm(mesh->mVertices[v]) * scale, 1.0f));
            out_v.Normal   = mesh->mNormals
                ? glm::normalize(normalMat * toGlm(mesh->mNormals[v]))
                : SVector3(0.0f, 1.0f, 0.0f);
            out_v.TexCoords = hasUV ? SVector2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y)
                                    : SVector2(0.0f);
            meshMin = glm::min(meshMin, out_v.Position);
            meshMax = glm::max(meshMax, out_v.Position);
        }

        // Gather influences per vertex.
        std::vector<std::vector<SBoneInfluence>> perVertex(vertexCount);
        for (unsigned int b = 0; b < mesh->mNumBones; ++b)
        {
            const aiBone* aiB = mesh->mBones[b];
            const int boneIndex = skeleton.findBone(SString(aiB->mName.C_Str()));
            if (boneIndex < 0)
            {
                MWARN(SString::format("Skeletal import: bone '{0}' has no node in {1} - its weights are dropped",
                                      SString(aiB->mName.C_Str()), path));
                continue;
            }

            SBone& bone = skeleton.bones[boneIndex];
            bone.isDeformBone = true;
            const SMatrix4 inverseBind = scaleTranslation(toGlm(aiB->mOffsetMatrix), scale) * invMeshGlobal;
            if (!inverseBindSet[boneIndex])
            {
                bone.inverseBind = inverseBind;
                inverseBindSet[boneIndex] = true;
            }
            else if (!warnedInverseBindMismatch && !matricesClose(bone.inverseBind, inverseBind))
            {
                warnedInverseBindMismatch = true;
                MWARN(STR("Skeletal import: meshes disagree on a bone's bind pose (first: '")
                      + bone.name + "') in " + path + " - the first mesh's is used");
            }

            for (unsigned int w = 0; w < aiB->mNumWeights; ++w)
            {
                const aiVertexWeight& vw = aiB->mWeights[w];
                if (vw.mVertexId < vertexCount && vw.mWeight > 0.0f)
                    perVertex[vw.mVertexId].push_back({ static_cast<uint32_t>(boneIndex), vw.mWeight });
            }
        }

        sub.offsets.resize(vertexCount + 1);
        sub.influences.reserve(vertexCount * 4);
        for (unsigned int v = 0; v < vertexCount; ++v)
        {
            auto& list = perVertex[v];
            sub.offsets[v] = static_cast<uint32_t>(sub.influences.size());

            if (list.empty())
            {
                // Rigid / unskinned vertex: follows the node holding its mesh.
                sub.influences.push_back({ static_cast<uint32_t>(inst.node), 1.0f });
                continue;
            }

            std::sort(list.begin(), list.end(),
                      [](const SBoneInfluence& x, const SBoneInfluence& y) { return x.weight > y.weight; });
            if (maxInfluences > 0 && static_cast<int>(list.size()) > maxInfluences)
                list.resize(maxInfluences);

            float total = 0.0f;
            for (const auto& infl : list) total += infl.weight;
            const float norm = total > 0.0f ? 1.0f / total : 1.0f;
            for (auto infl : list)
            {
                infl.weight *= norm;
                sub.influences.push_back(infl);
            }
        }
        sub.offsets[vertexCount] = static_cast<uint32_t>(sub.influences.size());

        // Triangles only - point/line elements would scramble a GL_TRIANGLES list.
        sub.indices.reserve(mesh->mNumFaces * 3);
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            sub.indices.push_back(face.mIndices[0]);
            sub.indices.push_back(face.mIndices[1]);
            sub.indices.push_back(face.mIndices[2]);
        }

        out.subMeshes.push_back(std::move(sub));
    }

    if (out.subMeshes.empty())
    {
        outError = "no meshes found";
        return false;
    }

    // Root deform bone: the first (parent-first order) whose ancestors
    // don't deform anything.
    for (int i = 0; i < boneCount && skeleton.rootBone < 0; ++i)
    {
        if (!skeleton.bones[i].isDeformBone) continue;
        bool hasDeformAncestor = false;
        for (int p = skeleton.bones[i].parent; p >= 0; p = skeleton.bones[p].parent)
            if (skeleton.bones[p].isDeformBone) { hasDeformAncestor = true; break; }
        if (!hasDeformAncestor)
            skeleton.rootBone = i;
    }
    if (skeleton.rootBone < 0 && boneCount > 0)
        skeleton.rootBone = 0;

    // Pivot -> root offset.
    SVector3 origin(0.0f);
    switch (settings.pivot)
    {
        case EMeshPivot::Center:
            origin = (meshMin + meshMax) * 0.5f;
            break;
        case EMeshPivot::BottomCenter:
            origin = SVector3((meshMin.x + meshMax.x) * 0.5f, meshMin.y, (meshMin.z + meshMax.z) * 0.5f);
            break;
        case EMeshPivot::FileOrigin:
        default:
            break;
    }
    skeleton.rootOffset = glm::translate(SMatrix4(1.0f), -origin);
    out.boundsMin = meshMin - origin;
    out.boundsMax = meshMax - origin;

    // Padding: how far the bind-pose skin reaches past the deform joints.
    std::vector<SMatrix4> bindModel;
    skeleton.computeBindModelTransforms(bindModel);
    bool anyDeform = false;
    for (const auto& bone : skeleton.bones) anyDeform |= bone.isDeformBone;

    SVector3 jointMin(std::numeric_limits<float>::max());
    SVector3 jointMax(std::numeric_limits<float>::lowest());
    for (int i = 0; i < boneCount; ++i)
    {
        if (anyDeform && !skeleton.bones[i].isDeformBone) continue;
        const SVector3 p(bindModel[i][3]);
        jointMin = glm::min(jointMin, p);
        jointMax = glm::max(jointMax, p);
    }
    const SVector3 size = out.boundsMax - out.boundsMin;
    out.jointBoundsPadding = glm::max(glm::max(out.boundsMax - jointMax, jointMin - out.boundsMin), SVector3(0.0f))
                           + size * 0.05f;
    return true;
}

// -- Public API -----------------------------------------------------------------

bool MSkeletalModelLoader::load(const SString& path, const SModelImportSettings& settings, EContent content,
                                 SSkeletalModelData& out, SString& outError)
{
    Assimp::Importer importer;
    configureImporter(importer);

    // No PreTransformVertices: it would discard the hierarchy and bones.
    // GlobalScale brings FBX centimetres to metres, scaling vertices,
    // bone offsets, node translations and position keys together.
    unsigned int flags = aiProcess_GlobalScale;
    if (content == EContent::MeshesAndClips)
        flags |= aiProcess_Triangulate
               | aiProcess_JoinIdenticalVertices
               | aiProcess_GenNormals
               | aiProcess_FlipUVs;

    const aiScene* scene = importer.ReadFile(path.c_str(), flags);
    if (!scene || !scene->mRootNode || (content == EContent::MeshesAndClips && (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)))
    {
        outError = SString("Assimp: ") + importer.GetErrorString();
        return false;
    }

    if (content == EContent::MeshesAndClips)
    {
        if (!buildSkeletonAndMeshes(scene, path, settings, out, outError))
            return false;
    }
    else if (scene->mNumAnimations == 0)
    {
        outError = "no animations found";
        return false;
    }

    buildClips(scene, path, settings, out.clips);
    return true;
}

EModelImportAs MSkeletalModelLoader::detect(const SString& path)
{
    Assimp::Importer importer;
    configureImporter(importer);

    const aiScene* scene = importer.ReadFile(path.c_str(), 0);
    if (!scene || !scene->mRootNode)
        return EModelImportAs::StaticMesh;   // the static importer reports the error

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
        if (scene->mMeshes[m]->HasBones())
            return EModelImportAs::SkeletalMesh;

    if (scene->mNumMeshes == 0 && scene->mNumAnimations > 0)
        return EModelImportAs::AnimationOnly;

    return EModelImportAs::StaticMesh;
}
