//
// ui_widget.cpp
//

#include "ui_widget.h"

#include <RmlUi/Core/Element.h>

#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"
#include "core/utils/logger.h"
#include "ui_widget_instance.h"

namespace
{
    MViewportSubsystem* ui()
    {
        return MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>();
    }
}

MUIWidget MUIWidget::create(TAssetHandle<MUIWidgetAsset> asset)
{
    auto* subsystem = ui();
    if (!subsystem)
    {
        MERROR("MUIWidget.create: the UI subsystem is not running");
        return {};
    }
    return subsystem->createWidget(asset);
}

MUIWidget MUIWidget::createFromReference(const SString& reference)
{
    auto* manager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    if (!manager)
        return {};
    const auto asset = manager->getAssetFromReference<MUIWidgetAsset>(reference);
    if (!asset.isValid())
    {
        MERROR("MUIWidget.create: '" + reference + "' is not a UI widget asset (.muia)");
        return {};
    }
    return create(asset);
}

SUIWidgetInstance* MUIWidget::resolve() const
{
    if (isNull())
        return nullptr;
    auto* subsystem = ui();
    return subsystem ? subsystem->resolve(*this) : nullptr;
}

bool MUIWidget::isValid() const
{
    return resolve() != nullptr;
}

void MUIWidget::addToViewport(int zOrder)
{
    if (auto* subsystem = ui())
        subsystem->addToViewport(*this, zOrder);
}

void MUIWidget::removeFromViewport()
{
    if (auto* subsystem = ui())
        subsystem->removeFromViewport(*this);
}

void MUIWidget::destroy()
{
    if (auto* subsystem = ui())
        subsystem->destroyWidget(*this);
}

bool MUIWidget::isInViewport() const
{
    const SUIWidgetInstance* widget = resolve();
    return widget && ui()->isShown(*widget);
}

int MUIWidget::getZOrder() const
{
    const SUIWidgetInstance* widget = resolve();
    if (!widget)
        return 0;
    const SUIWidgetInstance* top = ui()->getTopLevel(*widget);
    return top ? top->zOrder : 0;
}

void MUIWidget::setVisible(bool visible)
{
    if (auto* subsystem = ui())
        subsystem->setWidgetVisible(*this, visible);
}

bool MUIWidget::isVisible() const
{
    const SUIWidgetInstance* widget = resolve();
    return widget && widget->visible;
}

MUIElement MUIWidget::findElement(const SString& id) const
{
    // The lookup itself happens on use, so the handle survives a rebuild.
    if (!isValid())
        return {};
    return MUIElement(*this, id);
}

MUIWidget MUIWidget::findWidget(const SString& id) const
{
    const SUIWidgetInstance* widget = resolve();
    if (!widget)
        return {};
    const SUIWidgetInstance* child = ui()->findChildWidget(*widget, id);
    return child ? child->handle() : MUIWidget();
}

MUIWidget MUIWidget::getParent() const
{
    const SUIWidgetInstance* widget = resolve();
    if (!widget || widget->isTopLevel())
        return {};
    const SUIWidgetInstance* parent = ui()->getInstance(widget->parentId);
    return parent ? parent->handle() : MUIWidget();
}

void MUIWidget::setTickWhenPaused(bool tick)
{
    if (SUIWidgetInstance* widget = resolve())
        widget->tickWhenPaused = tick;
}

bool MUIWidget::getTickWhenPaused() const
{
    const SUIWidgetInstance* widget = resolve();
    return widget && widget->tickWhenPaused;
}

SString MUIWidget::getName() const
{
    const SUIWidgetInstance* widget = resolve();
    return widget ? widget->name : SString();
}
