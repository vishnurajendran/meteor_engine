//
// Cutout inputs - alpha cutout for depth-only passes.
//

#include <GL/glew.h>

#include "cutout_inputs.h"
#include "render_item.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/texture.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/shader/default_textures.h"

bool bindCutoutInputs(const SRenderItem& item, unsigned int program, int texUnit)
{
    const GLuint prog = program;
    const GLint useLoc = glGetUniformLocation(prog, "_useCutout");
    if (useLoc < 0) return false;   // program has no cutout support

    if (!item.material)
    {
        glUniform1i(useLoc, 0);
        return false;
    }

    const auto& props = item.material->getProperties();
    auto find = [&](const char* key) -> const SShaderPropertyValue*
    {
        const auto it = props.find(key);
        return it != props.end() ? &it->second : nullptr;
    };
    auto asBool = [](const SShaderPropertyValue* v, bool fallback)
    {
        if (!v) return fallback;
        return v->getType() == SShaderPropertyType::Int ? v->getIntVal() != 0 : v->getBoolValue();
    };

    const bool enabled = asBool(find("_useCutout"), false);
    glUniform1i(useLoc, enabled ? 1 : 0);
    if (!enabled) return false;

    GLint l;
    const auto* threshold = find("_cutoutThreshold");
    l = glGetUniformLocation(prog, "_cutoutThreshold");
    if (l >= 0) glUniform1f(l, threshold ? threshold->getFloatVal() : 0.5f);

    const auto* color = find("_color");
    const glm::vec4 c = color ? color->getColor() : glm::vec4(1.0f);
    l = glGetUniformLocation(prog, "_color");
    if (l >= 0) glUniform4f(l, c.r, c.g, c.b, c.a);

    l = glGetUniformLocation(prog, "_useAlbedoTex");
    if (l >= 0) glUniform1i(l, asBool(find("_useAlbedoTex"), true) ? 1 : 0);

    // Empty or missing -> white, so only _color's alpha counts.
    bool bound = false;
    if (const auto* albedo = find("_albedo"); albedo && !albedo->getTexAssetReference().empty())
    {
        const auto tex = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
            ->getAssetFromReference<MTextureAsset>(albedo->getTexAssetReference());
        if (tex && tex->getTexture())
        {
            tex->bindForRendering(texUnit);
            bound = true;
        }
    }
    if (!bound) MDefaultTextures::bindWhite(texUnit);

    l = glGetUniformLocation(prog, "_albedo");
    if (l >= 0) glUniform1i(l, texUnit);
    return true;
}
