//
// layer_filter.h
//

#ifndef LAYER_FILTER_H
#define LAYER_FILTER_H

#include "core/utils/sstring.h"

// SLayerFilter wraps a 32-bit bitmask where bit N means "include layer N".
//
// Layer indices map to names via MEngineStatics::getPhysicsLayers():
//   0 = "Default"      (all physics entities use this by default)
//   1 = "Unnamed_Layer_1" ... 31 = "Unnamed_Layer_31"
//
// SLayerFilter has no default constructor - always create via a factory method
// so the caller is explicit about which layers they want.
//
// Usage:
//   auto filter = SLayerFilter::createFromIndex(0);       // Default layer only
//   filter.addLayerIndex(2);                              // also include layer 2
//
//   auto allFilter = SLayerFilter::createAll();           // cast against everything
//   auto noneFilter = SLayerFilter::createEmpty();        // cast against nothing
class SLayerFilter
{
public:
    SLayerFilter()  = delete;
    ~SLayerFilter() = default;

    // ---- Query -------------------------------------------------------------

    // Returns the raw bitmask — bit N set means layer N is included.
    [[nodiscard]] unsigned int getLayerMask() const { return mask; }

    // ---- Mutation ----------------------------------------------------------

    // Adds a layer by index (0-31). Indices outside that range are masked to
    // fit — passing 32 is equivalent to passing 0.
    void addLayerIndex(unsigned int index) { mask |= (1u << (index & 31u)); }

    // Adds a layer by name. Requires MEngineStatics::getPhysicsLayers() to
    // return a valid asset. Silently does nothing if the asset is not loaded.
    void addLayerByName(const SString& layerName);

    // ---- Factory -----------------------------------------------------------

    // Include no layers. A raycast using this filter will always miss.
    static SLayerFilter createEmpty() { return SLayerFilter(0u); }

    // Include exactly one layer by index.
    static SLayerFilter createFromIndex(unsigned int index)
    {
        return SLayerFilter(1u << (index & 31u));
    }

    // Include exactly one layer by name.
    // Returns empty filter if the physics layers asset is not loaded.
    static SLayerFilter createFromName(const SString& layerName);

    // Include all 32 layers — equivalent to casting with no layer filter.
    static SLayerFilter createAll() { return SLayerFilter(~0u); }

    // Include only layer 0 ("Default"). Convenient shorthand for the most
    // common filter when no layer has been explicitly set.
    static SLayerFilter createDefault() { return createFromIndex(0u); }

private:
    explicit SLayerFilter(unsigned int mask) : mask(mask) {}

    unsigned int mask = 0u;
};

#endif // LAYER_FILTER_H