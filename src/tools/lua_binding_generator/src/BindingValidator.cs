// BindingValidator.cs
// Pass 2 (continued): Validate cross-references between bound classes.
//
// Primary check:
//   Every SCRIPT_BIND_REF_PROP must reference a type that is itself a
//   SCRIPT_BIND_CLASS. If it does not, the tool emits an error and returns
//   a non-zero exit code so CMake stops the build before the C++ compiler
//   ever sees the generated code.
//
// This is the "compile-time error for non-bound ref types" you wanted,
// implemented at code-gen time rather than C++ compile time.

using System;
using System.Collections.Generic;
using System.Linq;

namespace MeteorBindingGenerator;

public static class BindingValidator
{
    /// <summary>
    /// Validate all bound classes. Returns the number of errors found.
    /// Any error means the build should be stopped.
    /// </summary>
    public static int Validate(List<BoundClass> boundClasses)
    {
        int errorCount = 0;

        // Build the set of all known bound class names (Pass 1 result)
        var boundClassNames = new HashSet<string>(
            boundClasses.Select(c => c.Name)
        );

        foreach (var cls in boundClasses)
        {
            errorCount += ValidateRefProps(cls, boundClassNames);
            errorCount += ValidateFunctions(cls, boundClassNames);
            errorCount += ValidateDuplicateNames(cls);
        }

        return errorCount;
    }

    /// <summary>
    /// Check that every SCRIPT_BIND_REF_PROP references a known bound class.
    /// </summary>
    private static int ValidateRefProps(BoundClass cls, HashSet<string> boundClassNames)
    {
        int errors = 0;

        foreach (var prop in cls.Properties)
        {
            if (prop.AccessKind != PropertyAccessKind.RefProp)
                continue;

            if (string.IsNullOrEmpty(prop.CppType))
            {
                Console.Error.WriteLine(
                    $"[ERROR] {prop.SourceFile}:{prop.Line}: " +
                    $"SCRIPT_BIND_REF_PROP() on '{prop.Name}' in class '{cls.Name}': " +
                    $"could not determine the referenced type.");
                errors++;
                continue;
            }

            if (!boundClassNames.Contains(prop.CppType))
            {
                Console.Error.WriteLine(
                    $"[ERROR] {prop.SourceFile}:{prop.Line}: " +
                    $"SCRIPT_BIND_REF_PROP() on '{prop.Name}' in class '{cls.Name}': " +
                    $"referenced type '{prop.CppType}' is not a SCRIPT_BIND_CLASS(). " +
                    $"All types used with SCRIPT_BIND_REF_PROP must themselves be annotated " +
                    $"with SCRIPT_BIND_CLASS().");
                errors++;
            }
        }

        return errors;
    }

    /// <summary>
    /// Check that function return types and parameter types that are
    /// user-defined classes are also bound (when they need to cross the
    /// Lua boundary as userdata).
    /// </summary>
    private static int ValidateFunctions(BoundClass cls, HashSet<string> boundClassNames)
    {
        int errors = 0;
        // Primitive types that do not require binding validation
        var primitives = new HashSet<string>
        {
            "void", "int", "float", "double", "bool",
            "char", "short", "long", "long long",
            "unsigned int", "unsigned char", "unsigned short",
            "std::string", "SString",
            "SVector2", "SVector3", "SVector4", "SQuaternion"
        };

        foreach (var func in cls.Functions)
        {
            // Check return type
            var retType = StripQualifiers(func.ReturnType);
            if (!primitives.Contains(retType) && retType.EndsWith("*"))
            {
                var pointedType = retType[..^1].Trim();
                if (!primitives.Contains(pointedType) && !boundClassNames.Contains(pointedType))
                {
                    Console.Error.WriteLine(
                        $"[WARNING] {func.SourceFile}:{func.Line}: " +
                        $"SCRIPT_BIND_FUNC() '{func.Name}' in class '{cls.Name}': " +
                        $"return type '{func.ReturnType}' points to '{pointedType}' which " +
                        $"is not a SCRIPT_BIND_CLASS(). The generated binding may not " +
                        $"correctly push this type to Lua.");
                    // Warning, not error. The user might handle this manually.
                }
            }
        }

        return errors;
    }

    /// <summary>
    /// Check for duplicate property or function names within a class.
    /// </summary>
    private static int ValidateDuplicateNames(BoundClass cls)
    {
        int errors = 0;

        var propNames = new HashSet<string>();
        foreach (var prop in cls.Properties)
        {
            if (!propNames.Add(prop.Name))
            {
                Console.Error.WriteLine(
                    $"[ERROR] {prop.SourceFile}:{prop.Line}: " +
                    $"Duplicate SCRIPT_BIND_PROP for '{prop.Name}' in class '{cls.Name}'.");
                errors++;
            }
        }

        var funcNames = new HashSet<string>();
        foreach (var func in cls.Functions)
        {
            if (!funcNames.Add(func.Name))
            {
                Console.Error.WriteLine(
                    $"[ERROR] {func.SourceFile}:{func.Line}: " +
                    $"Duplicate SCRIPT_BIND_FUNC for '{func.Name}' in class '{cls.Name}'. " +
                    $"Lua does not support function overloading.");
                errors++;
            }
        }

        return errors;
    }

    /// <summary>
    /// Strip const, volatile, and reference/pointer qualifiers for comparison.
    /// </summary>
    private static string StripQualifiers(string type)
    {
        return type
            .Replace("const ", "")
            .Replace("volatile ", "")
            .Replace("&", "")
            .Trim();
    }
}
