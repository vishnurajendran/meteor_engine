//
// lua_script_registry.cpp
//

#include "lua_script_registry.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <regex>
#include <string>
#include <string_view>

#include "lua_script_asset.h"
#include "core/utils/logger.h"

std::unordered_map<SString, std::vector<MLuaScriptAsset*>> MLuaScriptRegistry::scriptsByName;
std::unordered_map<MLuaScriptAsset*, SString> MLuaScriptRegistry::nameByScript;
std::unordered_set<MLuaScriptAsset*> MLuaScriptRegistry::allScripts;
uint64_t MLuaScriptRegistry::generation = 0;

namespace
{
std::string trim(std::string_view s)
{
    size_t start = 0;
    size_t end   = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return std::string(s.substr(start, end - start));
}

bool isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

bool isIdentifier(std::string_view s)
{
    if (s.empty())
        return false;
    if (!std::isalpha(static_cast<unsigned char>(s[0])) && s[0] != '_')
        return false;
    return std::ranges::all_of(s, isIdentChar);
}

// "game.camera" - one or more identifiers joined by dots.
bool isDottedName(std::string_view s)
{
    size_t start = 0;
    while (true)
    {
        const size_t dot = s.find('.', start);
        if (!isIdentifier(s.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start)))
            return false;
        if (dot == std::string_view::npos)
            return true;
        start = dot + 1;
    }
}

// ---------------------------------------------------------------------------
// Masking - the parser reads code, never comments or string contents.
// ---------------------------------------------------------------------------

// Two copies of the source, the same length (newlines kept, so offsets map to
// the same lines):
//   code      - comments blanked; strings intact (names are read from them)
//   structure - comments AND string contents blanked; brackets and keywords
//               found here are real code
struct SMaskedSource
{
    std::string code;
    std::string structure;
};

void blank(std::string& text, size_t from, size_t to)
{
    for (size_t i = from; i < to && i < text.size(); ++i)
        if (text[i] != '\n' && text[i] != '\r')
            text[i] = ' ';
}

// Level of a Lua long bracket ([[, [=[, ...) starting at `at`, or -1.
int longBracketLevel(const std::string& s, size_t at)
{
    if (at >= s.size() || s[at] != '[')
        return -1;
    size_t i = at + 1;
    int level = 0;
    while (i < s.size() && s[i] == '=') { ++level; ++i; }
    return (i < s.size() && s[i] == '[') ? level : -1;
}

// Position just after the matching ]=*] - or the end of the text.
size_t longBracketEnd(const std::string& s, size_t contentStart, int level)
{
    const std::string close = "]" + std::string(level, '=') + "]";
    const size_t at = s.find(close, contentStart);
    return at == std::string::npos ? s.size() : at + close.size();
}

SMaskedSource maskSource(const std::string& s)
{
    SMaskedSource masked{s, s};
    size_t i = 0;
    while (i < s.size())
    {
        const char c = s[i];

        // -- line comment / --[[ block comment ]]
        if (c == '-' && i + 1 < s.size() && s[i + 1] == '-')
        {
            const int level = longBracketLevel(s, i + 2);
            size_t end;
            if (level >= 0)
                end = longBracketEnd(s, i + 2 + level + 2, level);
            else
            {
                end = s.find('\n', i);
                if (end == std::string::npos) end = s.size();
            }
            blank(masked.code, i, end);
            blank(masked.structure, i, end);
            i = end;
            continue;
        }

        // "string" / 'string' - quotes kept, contents blanked in structure
        if (c == '"' || c == '\'')
        {
            size_t j = i + 1;
            while (j < s.size() && s[j] != c && s[j] != '\n')
                j += (s[j] == '\\') ? 2 : 1;
            blank(masked.structure, i + 1, j);
            i = std::min(j + 1, s.size());
            continue;
        }

        // [[long string]]
        if (const int level = longBracketLevel(s, i); level >= 0)
        {
            const size_t end = longBracketEnd(s, i + level + 2, level);
            blank(masked.structure, i, end);
            i = end;
            continue;
        }
        ++i;
    }
    return masked;
}

int lineOf(const std::string& s, size_t offset)
{
    return 1 + static_cast<int>(std::count(s.begin(), s.begin() + static_cast<std::ptrdiff_t>(std::min(offset, s.size())), '\n'));
}

// Every match of `re` in masked.code that starts in real code (not inside a
// string - e.g. print("MODULE 'x'") must not count).
std::vector<std::smatch> codeMatches(const SMaskedSource& masked, const std::regex& re, int tokenGroup)
{
    std::vector<std::smatch> found;
    for (auto it = std::sregex_iterator(masked.code.begin(), masked.code.end(), re); it != std::sregex_iterator(); ++it)
    {
        const auto pos = static_cast<size_t>(it->position(tokenGroup));
        if (masked.structure[pos] == masked.code[pos])
            found.push_back(*it);
    }
    return found;
}

// ---------------------------------------------------------------------------
// Members - the top-level keys of the CLASS member table
// ---------------------------------------------------------------------------

bool isBlockOpener(std::string_view word)
{
    return word == "function" || word == "if" || word == "do" || word == "repeat";
}
bool isBlockCloser(std::string_view word) { return word == "end" || word == "until"; }

// "number time between frames" → "number"; "fun(a: x, b: y) desc" → "fun(a: x, b: y)".
// The type is the first token, but a token may contain spaces inside brackets.
std::string leadingType(const std::string& text)
{
    int depth = 0;
    for (size_t i = 0; i < text.size(); ++i)
    {
        const char c = text[i];
        if (c == '(' || c == '<' || c == '{' || c == '[') ++depth;
        else if ((c == ')' || c == '>' || c == '}' || c == ']') && depth > 0) --depth;
        else if (depth == 0 && std::isspace(static_cast<unsigned char>(c)))
            return text.substr(0, i);
    }
    return text;
}

// ---@param name type lines directly above `line` (1-based) in the original source.
std::map<std::string, std::string> paramTypesAbove(const std::vector<std::string>& lines, int line)
{
    static const std::regex paramRe(R"(^\s*---@param\s+([A-Za-z_]\w*|\.\.\.)\??\s+(.+?)\s*$)");
    std::map<std::string, std::string> types;
    for (int i = line - 2; i >= 0; --i)
    {
        const std::string trimmed = trim(lines[static_cast<size_t>(i)]);
        if (trimmed.rfind("---", 0) != 0)
            break;
        std::smatch m;
        if (std::regex_match(trimmed, m, paramRe))
            types.emplace(m[1].str(), leadingType(m[2].str()));
    }
    return types;
}

std::string functionType(const std::string& paramList, const std::string& className,
                         const std::map<std::string, std::string>& paramTypes)
{
    std::string signature = "fun(";
    bool first = true;
    size_t start = 0;
    while (start <= paramList.size())
    {
        const size_t comma = paramList.find(',', start);
        const std::string param = trim(std::string_view(paramList).substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        if (!param.empty())
        {
            std::string type = "any";
            if (param == "self")
                type = className;
            else if (const auto it = paramTypes.find(param); it != paramTypes.end())
                type = it->second;

            signature += (first ? "" : ", ") + param + ": " + type;
            first = false;
        }
        if (comma == std::string::npos)
            break;
        start = comma + 1;
    }
    return signature + ")";
}

// LuaLS type of a member's value, from the text that follows `name =`.
std::string valueType(const SMaskedSource& masked, size_t valueStart, const std::string& className,
                      const std::map<std::string, std::string>& paramTypes)
{
    const std::string& st = masked.structure;
    if (valueStart >= st.size())
        return "any";
    if (st.compare(valueStart, 8, "function") == 0 && (valueStart + 8 >= st.size() || !isIdentChar(st[valueStart + 8])))
    {
        const size_t open  = st.find('(', valueStart);
        const size_t close = open == std::string::npos ? std::string::npos : st.find(')', open);
        if (close == std::string::npos)
            return "function";
        return functionType(st.substr(open + 1, close - open - 1), className, paramTypes);
    }

    const char c = st[valueStart];
    if (std::isdigit(static_cast<unsigned char>(c)) || (c == '-' && valueStart + 1 < st.size() && std::isdigit(static_cast<unsigned char>(st[valueStart + 1]))))
        return "number";
    if (c == '"' || c == '\'' || longBracketLevel(masked.code, valueStart) >= 0)
        return "string";
    if (st.compare(valueStart, 4, "true") == 0 || st.compare(valueStart, 5, "false") == 0)
        return "boolean";
    if (c == '{')
        return "table";
    return "any";   // nil, expressions, calls - not knowable without running the script
}

// Walks the member table starting at the '{' at `open`. Only depth-0 keys of
// the form `name = value` count; brackets and function/if/do/repeat blocks
// raise the depth, so locals inside methods are never mistaken for members.
std::vector<SLuaScriptMember> extractMembers(const SMaskedSource& masked, const std::vector<std::string>& lines,
                                             size_t open, const std::string& className)
{
    const std::string& st = masked.structure;
    std::vector<SLuaScriptMember> members;
    int  depth       = 0;
    bool expectField = true;

    for (size_t i = open + 1; i < st.size(); ++i)
    {
        const char c = st[i];

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            size_t end = i;
            while (end < st.size() && isIdentChar(st[end])) ++end;
            const std::string_view word(st.data() + i, end - i);
            // `x.end` / `x:do` would be a field access, not a keyword
            const char before = i > 0 ? st[i - 1] : ' ';
            const bool isKeyword = before != '.' && before != ':';

            if (depth == 0 && expectField)
            {
                size_t eq = end;
                while (eq < st.size() && std::isspace(static_cast<unsigned char>(st[eq]))) ++eq;
                if (eq < st.size() && st[eq] == '=' && (eq + 1 >= st.size() || st[eq + 1] != '='))
                {
                    size_t value = eq + 1;
                    while (value < st.size() && std::isspace(static_cast<unsigned char>(st[value]))) ++value;
                    const auto paramTypes = paramTypesAbove(lines, lineOf(st, i));
                    members.push_back({SString(std::string(word)), SString(valueType(masked, value, className, paramTypes))});
                    expectField = false;
                    i = value - 1;   // continue at the value, so a `function` there opens a block
                    continue;
                }
                expectField = false;
            }

            if (isKeyword && isBlockOpener(word))
                ++depth;
            else if (isKeyword && isBlockCloser(word))
                --depth;
            i = end - 1;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c)))
            continue;

        if (c == '{' || c == '(' || c == '[')
            ++depth;
        else if (c == '}' || c == ')' || c == ']')
        {
            if (depth == 0)
                break;   // end of the member table
            --depth;
        }
        else if (depth == 0 && (c == ',' || c == ';'))
        {
            expectField = true;
            continue;
        }
        if (depth == 0)
            expectField = false;
    }
    return members;
}

SString location(const SString& scriptPath, int line)
{
    return SString::format("{0}:{1}", scriptPath, SString::fromInt(line));
}
} // namespace

// ---------------------------------------------------------------------------
// Declaration parsing
// ---------------------------------------------------------------------------

SLuaScriptDeclaration MLuaScriptRegistry::parseDeclaration(const SString& source, const SString& scriptPath)
{
    // Group 2 is the call's name token - checked to be real code, not text in
    // a string. The *CallRe patterns find every call (name followed by '(' or
    // a string), the others only the well-formed ones, so a malformed call is
    // reported instead of being silently skipped.
    static const std::regex moduleCallRe(R"((^|[^\w.:])(MODULE)\s*[("'\[])");
    static const std::regex moduleRe(R"((^|[^\w.:])(MODULE)\s*\(?\s*(["'])([^"'\n]*)\3)");
    static const std::regex classCallRe(R"((^|[^\w.:])(CLASS)\s*[("'\[])");
    static const std::regex classRe(R"((^|[^\w.:])(CLASS)\s*\(\s*(["'])([^"'\n]*)\3\s*,\s*([A-Za-z_][\w.]*)\s*\))");
    static const std::regex importRe(R"((^|[^\w.:])local\s+([A-Za-z_]\w*)\s*=\s*import\s*\(?\s*(["'])([^"'\n]*)\3)");

    const std::string text   = source.str();
    const SMaskedSource masked = maskSource(text);

    std::vector<std::string> lines;
    {
        size_t start = 0;
        while (start <= text.size())
        {
            const size_t nl = text.find('\n', start);
            lines.push_back(text.substr(start, nl == std::string::npos ? std::string::npos : nl - start));
            if (nl == std::string::npos) break;
            start = nl + 1;
        }
    }

    SLuaScriptDeclaration declaration;
    declaration.nameSpace     = DEFAULT_NAMESPACE;
    declaration.baseClassName = "Behaviour";
    bool importable = true;

    // ---- CLASS -------------------------------------------------------
    const auto classCalls = codeMatches(masked, classCallRe, 2);
    const auto classDecls = codeMatches(masked, classRe, 2);

    if (classCalls.empty())
    {
        MWARN(SString::format("{0}: no CLASS - the script works on entities but cannot be imported", scriptPath));
        return declaration;
    }
    if (classCalls.size() > 1)
    {
        MERROR(SString::format("{0}: CLASS is used more than once - a script defines exactly one class. "
                               "The script cannot be imported.",
                               location(scriptPath, lineOf(text, static_cast<size_t>(classCalls[1].position(2))))));
        return declaration;
    }
    if (classDecls.empty() || classDecls[0].position(2) != classCalls[0].position(2))
    {
        MERROR(SString::format("{0}: write CLASS(\"Name\", Base) with the name as a string literal and the base "
                               "as a plain name - it is read without running the script. The script cannot be imported.",
                               location(scriptPath, lineOf(text, static_cast<size_t>(classCalls[0].position(2))))));
        return declaration;
    }

    const std::smatch& classDecl = classDecls[0];
    const size_t classAt     = static_cast<size_t>(classDecl.position(2));
    const std::string className = classDecl[4].str();
    const std::string baseExpr  = classDecl[5].str();

    if (!isIdentifier(className))
    {
        MERROR(SString::format("{0}: CLASS(\"{1}\", ...) - the class name must be a plain identifier. "
                               "The script cannot be imported.",
                               location(scriptPath, lineOf(text, classAt)), SString(className)));
        importable = false;
    }

    // ---- MODULE -------------------------------------------------------------
    const auto moduleCalls = codeMatches(masked, moduleCallRe, 2);
    const auto moduleDecls = codeMatches(masked, moduleRe, 2);

    if (moduleCalls.size() > 1)
    {
        MERROR(SString::format("{0}: MODULE is used more than once. The script cannot be imported.",
                               location(scriptPath, lineOf(text, static_cast<size_t>(moduleCalls[1].position(2))))));
        importable = false;
    }
    else if (moduleCalls.size() == 1)
    {
        const auto at = static_cast<size_t>(moduleCalls[0].position(2));
        if (moduleDecls.empty() || static_cast<size_t>(moduleDecls[0].position(2)) != at)
        {
            MERROR(SString::format("{0}: write MODULE \"a.b\" with the name as a string literal. The script cannot be imported.",
                                   location(scriptPath, lineOf(text, at))));
            importable = false;
        }
        else if (at > classAt)
        {
            MERROR(SString::format("{0}: MODULE must come before CLASS. The script cannot be imported.",
                                   location(scriptPath, lineOf(text, at))));
            importable = false;
        }
        else if (!isDottedName(moduleDecls[0][4].str()))
        {
            MERROR(SString::format("{0}: invalid module '{1}' - use dotted identifiers like game.camera. "
                                   "The script cannot be imported.",
                                   location(scriptPath, lineOf(text, at)), SString(moduleDecls[0][4].str())));
            importable = false;
        }
        else
            declaration.nameSpace = moduleDecls[0][4].str();
    }

    if (!importable)
        return declaration;

    // ---- base class (for VS Code's types only) ------------------------------
    if (baseExpr != "Behaviour")
    {
        bool resolved = false;
        for (const auto& import : codeMatches(masked, importRe, 2))
        {
            if (import[2].str() != baseExpr)
                continue;
            // a full name ends in the class name: game.camera.CameraController
            const std::string fullName = import[4].str();
            declaration.baseClassName  = fullName.substr(fullName.find_last_of('.') + 1);
            resolved = true;
            break;
        }
        if (!resolved)
            MWARN(SString::format("{0}: base '{1}' is neither Behaviour nor a local from import() - the script runs, "
                                  "but VS Code will type it as a plain Behaviour",
                                  location(scriptPath, lineOf(text, classAt)), SString(baseExpr)));
    }

    // ---- members (for VS Code's types only) ---------------------------------
    // The member table follows the call: CLASS("Name", Base) { ... }
    size_t open = static_cast<size_t>(classDecl.position(0) + classDecl.length(0));
    while (open < masked.structure.size() && std::isspace(static_cast<unsigned char>(masked.structure[open]))) ++open;
    if (open < masked.structure.size() && masked.structure[open] == '(')
    {
        ++open;   // CLASS(...)({ ... })
        while (open < masked.structure.size() && std::isspace(static_cast<unsigned char>(masked.structure[open]))) ++open;
    }
    if (open < masked.structure.size() && masked.structure[open] == '{')
        declaration.members = extractMembers(masked, lines, open, className);

    declaration.className = className;
    declaration.fullName  = SString(declaration.nameSpace.str() + "." + className);
    return declaration;
}

// ---------------------------------------------------------------------------
// Index
// ---------------------------------------------------------------------------

void MLuaScriptRegistry::removeEntry(MLuaScriptAsset* asset)
{
    const auto it = nameByScript.find(asset);
    if (it == nameByScript.end())
        return;

    if (const auto listIt = scriptsByName.find(it->second); listIt != scriptsByName.end())
    {
        std::erase(listIt->second, asset);
        if (listIt->second.empty())
            scriptsByName.erase(listIt);
    }
    nameByScript.erase(it);
}

void MLuaScriptRegistry::registerScript(MLuaScriptAsset* asset)
{
    if (!asset)
        return;

    removeEntry(asset);
    allScripts.insert(asset);
    // Bumped even for scripts that are not importable - their compiled class
    // is cached by path and must be dropped too.
    ++generation;

    const SLuaScriptDeclaration& declaration = asset->getDeclaration();
    if (!declaration.isImportable())
        return;

    auto& scripts = scriptsByName[declaration.fullName];
    scripts.push_back(asset);
    nameByScript[asset] = declaration.fullName;

    if (scripts.size() > 1)
    {
        std::string paths;
        for (const auto* script : scripts)
            paths += "\n    " + script->getPath().str();
        MERROR(SString::format("'{0}' is declared by more than one script - import() refuses it until only one remains:{1}",
                               declaration.fullName, SString(paths)));
    }
}

void MLuaScriptRegistry::unregisterScript(MLuaScriptAsset* asset)
{
    if (!asset)
        return;

    removeEntry(asset);
    allScripts.erase(asset);
    ++generation;
}

std::vector<MLuaScriptAsset*> MLuaScriptRegistry::find(const SString& fullName)
{
    const auto it = scriptsByName.find(fullName);
    return it == scriptsByName.end() ? std::vector<MLuaScriptAsset*>{} : it->second;
}

std::vector<MLuaScriptAsset*> MLuaScriptRegistry::getAllScripts()
{
    std::vector<MLuaScriptAsset*> scripts(allScripts.begin(), allScripts.end());
    std::ranges::sort(scripts, [](const auto* a, const auto* b) { return a->getPath() < b->getPath(); });
    return scripts;
}

std::string MLuaScriptRegistry::buildTypeIndex()
{
    std::vector<const SLuaScriptDeclaration*> declarations;
    declarations.reserve(scriptsByName.size());
    for (const auto& [fullName, scripts] : scriptsByName)
        if (!scripts.empty())
            declarations.push_back(&scripts.front()->getDeclaration());

    std::ranges::sort(declarations, [](const auto* a, const auto* b) { return a->fullName < b->fullName; });

    std::string text =
        "---@meta\n"
        "-- script_index.lua\n"
        "-- AUTO-GENERATED by Meteorite from this project's scripts. DO NOT EDIT.\n"
        "--\n"
        "-- Declares every script class (base, cast, members) and import() with one\n"
        "-- overload per script, so VS Code (LuaLS) autocompletes names and members.\n"
        "\n";

    // A class block per script. The `local X = {}` ends each annotation block
    // so the next ---@class doesn't merge into it.
    for (const auto* declaration : declarations)
    {
        const std::string name = declaration->className.str();
        text += "---@class " + name + " : " + declaration->baseClassName.str() + "\n";
        text += "---@overload fun(obj: any): " + name + "|nil\n";
        for (const auto& member : declaration->members)
            text += "---@field " + member.name.str() + " " + member.luaType.str() + "\n";
        text += "local " + name + " = {}\n\n";
    }

    for (const auto* declaration : declarations)
        text += "---@overload fun(name: \"" + declaration->fullName.str() + "\"): " + declaration->className.str() + "\n";

    text +=
        "---@param name string full name of a script class, e.g. \"meteor.game.PlayerInput\"\n"
        "---@return table\n"
        "function import(name) end\n";
    return text;
}
