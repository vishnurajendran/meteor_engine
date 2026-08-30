//
// script_binding_macros.h
//
// Annotation macros for the Lua binding code generator.
// These expand to nothing at compile time. They serve as markers
// that the external MeteorBindingGenerator tool scans.
//

#ifndef SCRIPT_BINDING_MACROS_H
#define SCRIPT_BINDING_MACROS_H

#define SCRIPT_BIND_CLASS()
#define SCRIPT_BIND_STRUCT()
#define SCRIPT_BIND_PROP()
#define SCRIPT_BIND_REF_PROP()
#define SCRIPT_BIND_FUNC()
#define SCRIPT_BIND_FUNC_STATIC()
#define SCRIPT_BIND_ENUM()

#endif // SCRIPT_BINDING_MACROS_H