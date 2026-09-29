//
// asset_template_registry.h
//
// Data-driven "Create >" menu for the editor.
//
// A template turns (directory, name, params) into the contents of a new asset
// file. There are two kinds:
//
//   File templates       A text file in a templates directory with tokens
//                        such as __ASSET_NAME__ substituted. Shaders, skyboxes,
//                        scripts. Can be added without recompiling by listing
//                        them in <templates dir>/templates.xml.
//
//   Generated templates  A C++ function that builds the content, for assets
//                        whose content depends on other assets (a material
//                        needs its shader's property list).
//
// The registry only produces content. MEditorAssetManager::createAssetFromTemplate()
// picks a unique path, writes it through the asset source and registers it.
//
// Tokens available to file templates (all templates also get any params):
//   __ASSET_NAME__    the name typed by the user          ("My Enemy")
//   __CLASS_NAME__    name as a C/Lua identifier          ("My_Enemy")
//   __PROJECT_NAME__  set via setGlobalToken(), phase 4
//   __DATE__          YYYY-MM-DD
//   __<PARAM>__       every param, upper-cased key        (__SHADER__)
//
// Intended location: src/editor/editorassetmanager/asset_template_registry.h
//

#ifndef ASSET_TEMPLATE_REGISTRY_H
#define ASSET_TEMPLATE_REGISTRY_H

#include <functional>
#include <map>
#include <vector>

#include "core/utils/sstring.h"

// ---------------------------------------------------------------------------
// Parameters a template asks for before creation. The editor can build a
// creation dialog from these; templates with no params can be created
// straight from the menu.
// ---------------------------------------------------------------------------

enum class ETemplateParamKind
{
    Text,       // free text
    AssetRef,   // path of an existing asset; `assetExtension` filters the picker
    Enum,       // one of `options`
};

struct STemplateParam
{
    SString              key;             // "shader"
    SString              label;           // "Shader"
    ETemplateParamKind   kind = ETemplateParamKind::Text;
    SString              defaultValue;
    SString              assetExtension;  // AssetRef only, without dot: "shader"
    std::vector<SString> options;         // Enum only
    bool                 required = true;
};

// ---------------------------------------------------------------------------

struct STemplateContext
{
    SString                    directory;   // asset path of the target folder
    SString                    name;        // user-facing asset name
    std::map<SString, SString> params;      // filled from STemplateParam keys
    std::map<SString, SString> tokens;      // resolved tokens (see header comment)

    SString param(const SString& key, const SString& fallback = {}) const
    {
        auto it = params.find(key);
        return (it == params.end() || it->second.empty()) ? fallback : it->second;
    }
};

using FTemplateGenerator = std::function<bool(const STemplateContext& ctx, SString& outContent)>;

struct SAssetTemplate
{
    SString id;            // "shader.lit" - stable, used by code and templates.xml
    SString displayName;   // "Lit Shader"
    SString menuPath;      // "Shader/Lit" - '/' separates submenus
    SString extension;     // ".shader" (with dot)
    SString defaultName;   // "New_Shader"

    // Exactly one of these is used. If `generator` is set it wins.
    SString            templateFile;   // file name inside a templates directory
    FTemplateGenerator generator;

    std::vector<STemplateParam> params;
};

class MAssetTemplateRegistry
{
public:
    // -- Registration --------------------------------------------------------
    // Registering an id that already exists replaces it (later wins), so a
    // project can override an engine template.
    void registerTemplate(const SAssetTemplate& tmpl);
    void unregisterTemplate(const SString& id);
    void clear();

    // Directories searched for template files, in order; a file found in a
    // later directory overrides one in an earlier directory. Adding a
    // directory also loads its templates.xml, if present.
    void addTemplateDirectory(const SString& dir);
    void clearTemplateDirectories();

    // Tokens added to every template (e.g. __PROJECT_NAME__).
    void setGlobalToken(const SString& token, const SString& value) { globalTokens[token] = value; }

    // -- Queries ---------------------------------------------------------------
    [[nodiscard]] const SAssetTemplate* find(const SString& id) const;
    // In registration order, for building menus.
    [[nodiscard]] const std::vector<SAssetTemplate>& getAll() const { return templates; }

    // -- Generation ------------------------------------------------------------
    // Produces the file content for `id`. Fails (with a logged error) if the
    // template is unknown, a required param is missing, or the template file
    // cannot be read.
    bool generate(const SString& id, const SString& directory, const SString& name,
                  const std::map<SString, SString>& params, SString& outContent) const;

    // Replaces every token in `text`. Exposed so project scaffolding can reuse it.
    static SString applyTokens(const SString& text, const std::map<SString, SString>& tokens);

    // "My Enemy 2" -> "My_Enemy_2", "3d" -> "_3d"
    static SString toIdentifier(const SString& name);

private:
    bool loadTemplatesXml(const SString& dir);
    bool readTemplateFile(const SString& fileName, SString& out) const;
    STemplateContext makeContext(const SAssetTemplate& tmpl, const SString& directory,
                                 const SString& name, const std::map<SString, SString>& params) const;

    std::vector<SAssetTemplate> templates;
    std::vector<SString>        templateDirs;
    std::map<SString, SString>  globalTokens;
};

#endif // ASSET_TEMPLATE_REGISTRY_H
