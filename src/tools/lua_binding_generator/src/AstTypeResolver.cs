// AstTypeResolver.cs
// Pass 2 (partial): Use CppAst.NET (libclang) to parse annotated headers
// and resolve full type information for members found by the text scanner.
//
// Why we need this in addition to the text scanner:
//   The text scanner extracts names reliably, but regex cannot safely resolve
//   C++ types involving templates typedefs (SVector3 -> glm::vec3),
//   or complex function signatures. CppAst gives us the real AST for that.
//
// How it works:
//   1. Collect all source files that contain bound classes
//   2. Parse them with CppAst using stub headers (so we do not need SFML, ImGui, etc.)
//   3. For each bound class, look up its fields/functions in the AST
//   4. Overwrite the text-scanned type strings with the AST-resolved ones

using System;
using System.Collections.Generic;
using System.Linq;
using CppAst;

namespace MeteorBindingGenerator;

public static class AstTypeResolver
{
    /// <summary>
    /// Parse source files with CppAst and refine the type information
    /// in the bound classes that the annotation scanner found.
    /// </summary>
    /// <param name="boundClasses">Classes from the annotation scanner (modified in place).</param>
    /// <param name="sourceDir">Root source directory for include resolution.</param>
    /// <param name="stubDir">Directory containing stub headers.</param>
    /// <returns>Number of errors encountered during parsing.</returns>
    public static int ResolveTypes(List<BoundClass> boundClasses, string sourceDir, string stubDir)
    {
        int errorCount = 0;

        // Collect unique source files that contain bound classes
        var sourceFiles = boundClasses
            .Select(c => c.SourceFile)
            .Distinct()
            .ToList();

        if (sourceFiles.Count == 0)
            return 0;

        // Configure CppAst parser
        var options = new CppParserOptions();

        // Stub directory takes priority so our macro stubs shadow the real headers.
        // Added as BOTH regular and system include so it resolves:
        //   #include "field.h"      (regular include, from our stubs)
        //   #include <vector>       (system include, hits our std stubs)
        options.IncludeFolders.Add(stubDir);
        options.SystemIncludeFolders.Add(stubDir);

        // Engine source root for resolving internal #includes
        options.IncludeFolders.Add(sourceDir);

        // Common defines the engine expects
        options.Defines.Add("SCRIPT_BIND_CLASS()=");
        options.Defines.Add("SCRIPT_BIND_PROP()=");
        options.Defines.Add("SCRIPT_BIND_REF_PROP()=");
        options.Defines.Add("SCRIPT_BIND_FUNC()=");

        // Suppress warnings from the stub environment
        options.AdditionalArguments.Add("-Wno-everything");

        // Parse all relevant headers in one compilation unit
        // so cross-references between classes resolve correctly
        var compilation = CppParser.ParseFiles(sourceFiles, options);

        // Report parse errors (but do not fail -- partial results are still useful)
        if (compilation.HasErrors)
        {
            foreach (var msg in compilation.Diagnostics.Messages)
            {
                if (msg.Type == CppLogMessageType.Error)
                {
                    Console.Error.WriteLine($"[AST WARNING] {msg}");
                }
            }
        }

        // Build a lookup: className -> CppClass from the AST
        var astClasses = new Dictionary<string, CppClass>();
        CollectClasses(compilation.Classes, astClasses);

        // Refine each bound class with AST info
        foreach (var boundClass in boundClasses)
        {
            if (!astClasses.TryGetValue(boundClass.Name, out var astClass))
            {
                Console.Error.WriteLine(
                    $"[AST WARNING] Class '{boundClass.Name}' not found in AST. " +
                    $"Type resolution will use text-scanned types.");
                continue;
            }

            RefineProperties(boundClass, astClass);
            RefineFunctions(boundClass, astClass);
        }

        return errorCount;
    }

    /// <summary>
    /// Recursively collect all classes from the AST into a flat dictionary.
    /// CppAst nests inner classes, so we walk the tree.
    /// </summary>
    private static void CollectClasses(CppContainerList<CppClass> classes,
                                        Dictionary<string, CppClass> output)
    {
        foreach (var cls in classes)
        {
            if (!string.IsNullOrEmpty(cls.Name) && !output.ContainsKey(cls.Name))
                output[cls.Name] = cls;

            // Recurse into nested classes
            if (cls.Classes.Count > 0)
                CollectClasses(cls.Classes, output);
        }
    }

    /// <summary>
    /// Match bound properties to AST fields and refine their type strings.
    /// </summary>
    private static void RefineProperties(BoundClass boundClass, CppClass astClass)
    {
        foreach (var prop in boundClass.Properties)
        {
            // Find the matching field in the AST by name
            var astField = astClass.Fields.FirstOrDefault(f => f.Name == prop.Name);
            if (astField == null)
                continue;

            var resolvedType = ResolveTypeName(astField.Type);

            // For DECLARE_FIELD members, the AST sees Field<T>.
            // Extract the inner type T.
            if (prop.AccessKind == PropertyAccessKind.DeclareField &&
                resolvedType.StartsWith("Field<") && resolvedType.EndsWith(">"))
            {
                prop.CppType = resolvedType[6..^1].Trim();
            }
            // For ref props, extract the inner type from M<T> or similar smart pointer
            else if (prop.AccessKind == PropertyAccessKind.RefProp)
            {
                var inner = ExtractTemplateArg(resolvedType);
                if (inner != null)
                    prop.CppType = inner;
                else
                    prop.CppType = resolvedType;
            }
            else
            {
                prop.CppType = resolvedType;
            }
        }
    }

    /// <summary>
    /// Match bound functions to AST methods and refine their signatures.
    /// </summary>
    private static void RefineFunctions(BoundClass boundClass, CppClass astClass)
    {
        foreach (var func in boundClass.Functions)
        {
            var astFunc = astClass.Functions.FirstOrDefault(f => f.Name == func.Name);
            if (astFunc == null)
                continue;

            func.ReturnType = ResolveTypeName(astFunc.ReturnType);

            var newParams = new List<(string Type, string Name)>();
            foreach (var p in astFunc.Parameters)
            {
                newParams.Add((ResolveTypeName(p.Type), p.Name)!);
            }
            func.Parameters = newParams;
        }
    }

    /// <summary>
    /// Convert a CppType to a readable C++ type name string.
    /// </summary>
    private static string? ResolveTypeName(CppType type)
    {
        return type switch
        {
            CppPrimitiveType prim => prim.Kind switch
            {
                CppPrimitiveKind.Void => "void",
                CppPrimitiveKind.Bool => "bool",
                CppPrimitiveKind.Char => "char",
                CppPrimitiveKind.Short => "short",
                CppPrimitiveKind.Int => "int",
                CppPrimitiveKind.LongLong => "long long",
                CppPrimitiveKind.UnsignedInt => "unsigned int",
                CppPrimitiveKind.Float => "float",
                CppPrimitiveKind.Double => "double",
                _ => prim.ToString()
            },
            CppPointerType ptr => ResolveTypeName(ptr.ElementType) + "*",
            CppReferenceType refType => ResolveTypeName(refType.ElementType) + "&",
            CppQualifiedType qual => (qual.Qualifier == CppTypeQualifier.Const ? "const " : "")
                                      + ResolveTypeName(qual.ElementType),
            CppTypedef td => td.Name,
            CppClass cls => cls.Name,
            CppEnum en => en.Name,
            _ => type.ToString()
        };
    }

    /// <summary>
    /// Extract the first template argument from a type string like "M&lt;SomeObject&gt;".
    /// </summary>
    private static string? ExtractTemplateArg(string typeStr)
    {
        int open = typeStr.IndexOf('<');
        int close = typeStr.LastIndexOf('>');
        if (open < 0 || close <= open)
            return null;
        return typeStr[(open + 1)..close].Trim();
    }
}