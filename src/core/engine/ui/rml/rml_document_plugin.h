//
// rml_document_plugin.h
//
// Hands RmlUi's document-load notification to a callback. Context::LoadDocument
// calls it after parsing, before the document's first update (layout, text),
// so the viewport subsystem can give the document its stylesheet in time.
//

#ifndef RML_DOCUMENT_PLUGIN_H
#define RML_DOCUMENT_PLUGIN_H

#include <functional>
#include <utility>

#include <RmlUi/Core/Plugin.h>

namespace Rml { class ElementDocument; }

class MRmlDocumentPlugin : public Rml::Plugin
{
public:
    using FOnLoad = std::function<void(Rml::ElementDocument*)>;

    explicit MRmlDocumentPlugin(FOnLoad onLoad) : onLoad(std::move(onLoad)) {}

    int GetEventClasses() override { return EVT_DOCUMENT; }

    void OnDocumentLoad(Rml::ElementDocument* document) override
    {
        if (onLoad)
            onLoad(document);
    }

private:
    FOnLoad onLoad;
};

#endif // RML_DOCUMENT_PLUGIN_H
