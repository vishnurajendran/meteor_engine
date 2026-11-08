//
// Cutout inputs - alpha cutout for depth-only passes.
//

#pragma once
#ifndef CUTOUT_INPUTS_H
#define CUTOUT_INPUTS_H

struct SRenderItem;

// Materials with _useCutout discard pixels whose albedo alpha is below
// _cutoutThreshold. Passes that write depth with their own shader (shadow
// maps) must drop the same pixels, or a leaf quad casts a solid square.
//
// Uploads to `program` (uniforms it doesn't declare are skipped):
//   int       _useCutout         0 / 1
//   float     _cutoutThreshold
//   vec4      _color             alpha multiplies the texture's
//   int       _useAlbedoTex      lit has this toggle; unlit always samples (1)
//   sampler2D _albedo            bound on `texUnit`, white when unassigned
//
// Returns true when cutout is on for this item (a texture is then bound on
// `texUnit`; the caller unbinds it after drawing).
bool bindCutoutInputs(const SRenderItem& item, unsigned int program, int texUnit);

#endif // CUTOUT_INPUTS_H
