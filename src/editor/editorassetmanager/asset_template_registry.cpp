//
// asset_template_registry.cpp
//
// Intended location: src/editor/editorassetmanager/asset_template_registry.cpp
//

#include "asset_template_registry.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/utils/logger.h"
#include "pugixml.hpp"

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void MAssetTemplateRegistry::registerTemplate(const SAssetTemplate& tmpl)
{
    if (tmpl.id.empty())
    {
        MERROR("AssetTemplateRegistry:: template with empty id ignored");
        return;
    }

    for (auto& existing : templates)
    {
        if (existing.id == tmpl.id)
        {
            existing = tmpl;   // override in place, keeps menu order
            return;
        }
    }
    templates.push_back(tmpl);
}

void MAssetTemplateRegistry::unregisterTemplate(const SString& id)
{
    std::erase_if(templates, [&](const SAssetTemplate& t) { return t.id == id; });
}

void MAssetTemplateRegistry::clear()
{
    templates.clear();
}

void MAssetTemplateRegistry::addTemplateDirectory(const SString& dir)
{
    if (dir.empty()) return;
    if (std::find(templateDirs.begin(), templateDirs.end(), dir) == templateDirs.end())
        templateDirs.push_back(dir);
    loadTemplatesXml(dir);
}

void MAssetTemplateRegistry::clearTemplateDirectories()
{
    templateDirs.clear();
}

const SAssetTemplate* MAssetTemplateRegistry::find(const SString& id) const
{
    for (const auto& t : templates)
        if (t.id == id)
            return &t;
    return nullptr;
}

// ---------------------------------------------------------------------------
// templates.xml
//
// <templates>
//   <template id="script.lua" name="Lua Script" menu="Script/Lua"
//             file="script.lua.template" extension=".lua" defaultName="NewScript"/>
//   <template id="shader.water" name="Water Shader" menu="Shader/Water"
//             file="water.shader.template" extension=".shader" defaultName="New_Water">
//     <param key="tint" label="Tint" default="(0,0.3,0.5,1)" required="false"/>
//   </template>
// </templates>
// ---------------------------------------------------------------------------

bool MAssetTemplateRegistry::loadTemplatesXml(const SString& dir)
{
    const std::filesystem::path xmlPath = std::filesystem::path(dir.str()) / "templates.xml";
    std::error_code ec;
    if (!std::filesystem::exists(xmlPath, ec))
        return false;

    pugi::xml_document doc;
    const auto res = doc.load_file(xmlPath.c_str());
    if (!res)
    {
        MERROR(SString::format("AssetTemplateRegistry:: failed to parse {0}: {1}",
                               SString(xmlPath.string()), SString(res.description())));
        return false;
    }

    int count = 0;
    for (auto node : doc.child("templates").children("template"))
    {
        SAssetTemplate t;
        t.id           = node.attribute("id").as_string();
        t.displayName  = node.attribute("name").as_string(t.id.c_str());
        t.menuPath     = node.attribute("menu").as_string(t.displayName.c_str());
        t.templateFile = node.attribute("file").as_string();
        t.extension    = node.attribute("extension").as_string();
        t.defaultName  = node.attribute("defaultName").as_string("New_Asset");

        if (t.id.empty() || t.templateFile.empty() || t.extension.empty())
        {
            MWARN(SString::format("AssetTemplateRegistry:: {0}: template needs id, file and extension",
                                  SString(xmlPath.string())));
            continue;
        }
        if (t.extension.str().front() != '.')
            t.extension = SString(".") + t.extension;

        for (auto p : node.children("param"))
        {
            STemplateParam param;
            param.key          = p.attribute("key").as_string();
            param.label        = p.attribute("label").as_string(param.key.c_str());
            param.defaultValue = p.attribute("default").as_string();
            param.required     = p.attribute("required").as_bool(true);

            const SString kind = p.attribute("kind").as_string("text");
            if (kind == "asset")
            {
                param.kind           = ETemplateParamKind::AssetRef;
                param.assetExtension = p.attribute("assetExtension").as_string();
            }
            else if (kind == "enum")
            {
                param.kind = ETemplateParamKind::Enum;
                for (const auto& opt : SString(p.attribute("options").as_string()).split(","))
                    if (!opt.empty()) param.options.push_back(opt);
            }

            if (!param.key.empty())
                t.params.push_back(param);
        }

        registerTemplate(t);
        ++count;
    }

    MLOG(SString::format("AssetTemplateRegistry:: loaded {0} template(s) from {1}",
                         SString::fromInt(count), SString(xmlPath.string())));
    return true;
}

// ---------------------------------------------------------------------------
// Generation
// ---------------------------------------------------------------------------

bool MAssetTemplateRegistry::readTemplateFile(const SString& fileName, SString& out) const
{
    // Later directories override earlier ones.
    for (auto it = templateDirs.rbegin(); it != templateDirs.rend(); ++it)
    {
        const std::filesystem::path p = std::filesystem::path(it->str()) / fileName.str();
        std::ifstream ifs(p, std::ios::binary);
        if (!ifs.is_open())
            continue;

        std::ostringstream ss;
        ss << ifs.rdbuf();
        out = SString(ss.str());
        return true;
    }
    return false;
}

static SString todayString()
{
    const std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return SString(buf);
}

STemplateContext MAssetTemplateRegistry::makeContext(const SAssetTemplate& tmpl, const SString& directory,
                                                     const SString& name,
                                                     const std::map<SString, SString>& params) const
{
    STemplateContext ctx;
    ctx.directory = directory;
    ctx.name      = name.empty() ? tmpl.defaultName : name;

    // Defaults first, then what the caller supplied.
    for (const auto& p : tmpl.params)
        if (!p.defaultValue.empty())
            ctx.params[p.key] = p.defaultValue;
    for (const auto& [k, v] : params)
        ctx.params[k] = v;

    ctx.tokens = globalTokens;
    ctx.tokens["__ASSET_NAME__"] = ctx.name;
    ctx.tokens["__CLASS_NAME__"] = toIdentifier(ctx.name);
    ctx.tokens["__DATE__"]       = todayString();
    for (const auto& [k, v] : ctx.params)
    {
        std::string upper = k.str();
        std::transform(upper.begin(), upper.end(), upper.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        ctx.tokens[SString("__" + upper + "__")] = v;
    }
    return ctx;
}

bool MAssetTemplateRegistry::generate(const SString& id, const SString& directory, const SString& name,
                                      const std::map<SString, SString>& params, SString& outContent) const
{
    const SAssetTemplate* tmpl = find(id);
    if (!tmpl)
    {
        MERROR(SString("AssetTemplateRegistry:: unknown template ") + id);
        return false;
    }

    const STemplateContext ctx = makeContext(*tmpl, directory, name, params);

    for (const auto& p : tmpl->params)
    {
        if (p.required && ctx.param(p.key).empty())
        {
            MERROR(SString::format("AssetTemplateRegistry:: template {0} needs parameter '{1}'", id, p.key));
            return false;
        }
    }

    if (tmpl->generator)
        return tmpl->generator(ctx, outContent);

    SString raw;
    if (!readTemplateFile(tmpl->templateFile, raw))
    {
        MERROR(SString::format("AssetTemplateRegistry:: template file not found: {0} (template {1})",
                               tmpl->templateFile, id));
        return false;
    }

    outContent = applyTokens(raw, ctx.tokens);
    return true;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

SString MAssetTemplateRegistry::applyTokens(const SString& text, const std::map<SString, SString>& tokens)
{
    SString out = text;
    for (const auto& [token, value] : tokens)
        out.replace(token, value);
    return out;
}

SString MAssetTemplateRegistry::toIdentifier(const SString& name)
{
    std::string out;
    out.reserve(name.str().size() + 1);
    for (unsigned char c : name.str())
        out.push_back((std::isalnum(c) || c == '_') ? static_cast<char>(c) : '_');

    if (out.empty() || std::isdigit(static_cast<unsigned char>(out.front())))
        out.insert(out.begin(), '_');
    return SString(out);
}
