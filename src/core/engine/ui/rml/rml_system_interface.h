//
// rml_system_interface.h
//
// RmlUi's system interface: time from the engine clock, RmlUi's log into the
// engine log, the clipboard through SFML, cursor shapes through the main
// viewport's host, and asset-path aware path joining.
//

#ifndef RML_SYSTEM_INTERFACE_H
#define RML_SYSTEM_INTERFACE_H

#include <chrono>

#include <RmlUi/Core/SystemInterface.h>

class MRmlSystemInterface : public Rml::SystemInterface
{
public:
    MRmlSystemInterface();

    double GetElapsedTime() override;
    bool   LogMessage(Rml::Log::Type type, const Rml::String& message) override;
    void   SetMouseCursor(const Rml::String& cursorName) override;
    void   SetClipboardText(const Rml::String& text) override;
    void   GetClipboardText(Rml::String& text) override;

    // "assets/...", "meteor_assets/..." and "guid:..." are asset references
    // and are used as they are; anything else is relative to the document.
    void JoinPath(Rml::String& translatedPath, const Rml::String& documentPath, const Rml::String& path) override;

    static bool isAssetReference(const Rml::String& path);

private:
    std::chrono::steady_clock::time_point start;
};

#endif // RML_SYSTEM_INTERFACE_H
