//
// shape_axis.h
//

#ifndef SHAPE_AXIS_H
#define SHAPE_AXIS_H

// Axis along which a cylindrical or capsule shape is oriented.
// Jolt's native axis for both CylinderShape and CapsuleShape is Y.
// X and Z are achieved by wrapping the shape in a RotatedTranslatedShape
// at creation time — zero runtime overhead, one-time setup cost.
enum class EShapeAxis { X, Y, Z };

#endif // SHAPE_AXIS_H