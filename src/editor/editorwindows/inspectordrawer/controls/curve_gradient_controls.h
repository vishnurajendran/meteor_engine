//
// curve_gradient_controls.h
//
// Inspector widgets for SCurve and SGradient (core/utils/curve.h).
//
// Each draws a one-line preview that fills the value column; clicking it
// opens a popup editor:
//
//   Curve:    drag keys, double-click to add, right-click a key to remove,
//             exact Time / Value for the selected key, and the vertical
//             range of the graph.
//   Gradient: drag markers under the bar, double-click the bar to add,
//             right-click a marker to remove, colour picker for the
//             selected key.
//
// Usage, inside a field row:
//
//   START_FIELD_DRAW("Size Over Lifetime")
//       SCurve c = module->sizeCurve.get();
//       if (MCurveGradientControls::curveField("##v", c)) module->sizeCurve.set(c);
//   END_FIELD_DRAW()
//
// Both return true on the frames the value changed. Editor state (selected
// key, drag, graph range) lives in ImGui's per-window state storage, keyed
// by the widget's ID — nothing to keep alive on the caller's side.
//
#pragma once
#ifndef METEOR_ENGINE_CURVE_GRADIENT_CONTROLS_H
#define METEOR_ENGINE_CURVE_GRADIENT_CONTROLS_H

#include "core/utils/curve.h"

namespace MCurveGradientControls
{
    // viewMin / viewMax — the graph's initial vertical range. It grows to
    // fit the keys when the popup opens and can be changed inside it.
    bool curveField(const char* id, SCurve& curve, float viewMin = 0.0f, float viewMax = 1.0f);

    bool gradientField(const char* id, SGradient& gradient);
}

#endif // METEOR_ENGINE_CURVE_GRADIENT_CONTROLS_H
