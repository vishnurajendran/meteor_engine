//
// rml_system_interface.cpp
//

#include "rml_system_interface.h"

#include <RmlUi/Core/Log.h>

#include "SFML/System/String.hpp"
#include "SFML/Window/Clipboard.hpp"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/ui/viewport/viewport.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"
#include "core/utils/logger.h"

MRmlSystemInterface::MRmlSystemInterface() : start(std::chrono::steady_clock::now()) {}

double MRmlSystemInterface::GetElapsedTime()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

bool MRmlSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
    const SString text = SString("RmlUi: ") + message;
    switch (type)
    {
        case Rml::Log::LT_ERROR:
        case Rml::Log::LT_ASSERT:
            MERROR(text);
            break;
        case Rml::Log::LT_WARNING:
            MWARN(text);
            break;
        case Rml::Log::LT_DEBUG:
            MVERBOSE(text);
            break;
        default:
            MLOG(text);
            break;
    }
    return true;   // never break into the debugger
}

void MRmlSystemInterface::SetMouseCursor(const Rml::String& cursorName)
{
    auto* ui = MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>();
    MViewport* viewport = ui ? ui->getMainViewport() : nullptr;
    if (viewport && viewport->getHost())
        viewport->getHost()->setCursorShape(SString(cursorName));
}

void MRmlSystemInterface::SetClipboardText(const Rml::String& text)
{
    sf::Clipboard::setString(sf::String::fromUtf8(text.begin(), text.end()));
}

void MRmlSystemInterface::GetClipboardText(Rml::String& text)
{
    const auto utf8 = sf::Clipboard::getString().toUtf8();
    text.assign(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}

bool MRmlSystemInterface::isAssetReference(const Rml::String& path)
{
    auto startsWith = [&](const char* prefix)
    {
        const Rml::String p(prefix);
        return path.size() >= p.size() && path.compare(0, p.size(), p) == 0;
    };
    return startsWith(ASSET_REF_GUID_PREFIX) || startsWith("assets/") || startsWith(MAssetManager::ENGINE_ASSET_ROUTE);
}

void MRmlSystemInterface::JoinPath(Rml::String& translatedPath, const Rml::String& documentPath, const Rml::String& path)
{
    if (isAssetReference(path))
    {
        translatedPath = path;
        return;
    }
    Rml::SystemInterface::JoinPath(translatedPath, documentPath, path);
}
