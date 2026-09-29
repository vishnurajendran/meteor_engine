// AnnotationScanner.cs
// Pass 1: Text-based pre-scan of C++ headers.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace MeteorBindingGenerator;

public static class AnnotationScanner
{
    private const string ClassMarker   = "SCRIPT_BIND_CLASS()";
    private const string PropMarker    = "SCRIPT_BIND_PROP()";
    private const string RefPropMarker = "SCRIPT_BIND_REF_PROP()";
    private const string FuncMarker       = "SCRIPT_BIND_FUNC()";
    private const string StaticFuncMarker  = "SCRIPT_BIND_FUNC_STATIC()";
    private const string EnumMarker        = "SCRIPT_BIND_ENUM()";
    private const string StructMarker  = "SCRIPT_BIND_STRUCT()";

    private static readonly Regex ClassNamePattern =
        new(@"^\s*(?:class|struct)\s+(\w+)", RegexOptions.Compiled);

    private static readonly Regex DeclareFieldPattern =
        new(@"DECLARE_FIELD\s*\(\s*(\w+)\s*,\s*(.+?)\s*,", RegexOptions.Compiled);

    private static readonly Regex RawFieldPattern =
        new(@"^\s*([\w:<>&*\s]+?)\s+(\w+)\s*[;=]", RegexOptions.Compiled);

    private static readonly Regex FuncPattern =
        new(@"^\s*([\w:<>&*\s]+?)\s+(\w+)\s*\(([^)]*)\)", RegexOptions.Compiled);

    private static readonly Regex CppAttributePattern =
        new(@"\[\[.*?\]\]", RegexOptions.Compiled);

    // Matches: "enum class EName" or "enum EName"
    // Group 1 = "class" (optional), Group 2 = enum name
    private static readonly Regex EnumPattern =
        new(@"^\s*enum\s+(class\s+)?(\w+)", RegexOptions.Compiled);

    /// <summary>
    /// Scan results: classes and standalone enums.
    /// </summary>
    public record ScanResult(List<BoundClass> Classes, List<BoundEnum> Enums);

    public static ScanResult ScanDirectory(string sourceDir)
    {
        var classes = new List<BoundClass>();
        var enums = new List<BoundEnum>();
        var headerFiles = Directory.GetFiles(sourceDir, "*.h", SearchOption.AllDirectories);

        foreach (var file in headerFiles)
        {
            var result = ScanFile(file);
            classes.AddRange(result.Classes);
            enums.AddRange(result.Enums);
        }

        return new ScanResult(classes, enums);
    }

    public static ScanResult ScanFile(string filePath)
    {
        var classes = new List<BoundClass>();
        var enums = new List<BoundEnum>();
        var lines = File.ReadAllLines(filePath);

        BoundClass? currentClass = null;

        for (int i = 0; i < lines.Length; i++)
        {
            var trimmed = lines[i].Trim();

            if (string.IsNullOrEmpty(trimmed) || trimmed.StartsWith("//"))
                continue;

            // -- SCRIPT_BIND_ENUM: can appear inside or outside a class
            if (trimmed.StartsWith(EnumMarker))
            {
                var boundEnum = ParseEnum(lines, i + 1, filePath, i + 1);
                if (boundEnum != null)
                    enums.Add(boundEnum);
                else
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: SCRIPT_BIND_ENUM() not followed by a valid enum declaration.");
                continue;
            }

            // -- SCRIPT_BIND_CLASS
            if (trimmed.StartsWith(ClassMarker) || trimmed.StartsWith(StructMarker))
            {
                var nextDecl = FindNextDeclaration(lines, i + 1);
                if (nextDecl == null)
                {
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: SCRIPT_BIND_CLASS() with no class declaration following it.");
                    continue;
                }

                var nameMatch = ClassNamePattern.Match(nextDecl);
                if (!nameMatch.Success)
                {
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: SCRIPT_BIND_CLASS() not followed by a class declaration. " +
                        $"Found: \"{nextDecl.Trim()}\"");
                    continue;
                }

                currentClass = new BoundClass
                {
                    Name = nameMatch.Groups[1].Value,
                    ParentClasses = ExtractParentClasses(nextDecl),
                    SourceFile = filePath
                };
                classes.Add(currentClass);
                continue;
            }

            if (currentClass == null)
                continue;

            // -- SCRIPT_BIND_PROP
            if (trimmed.StartsWith(PropMarker))
            {
                var nextDecl = FindNextDeclaration(lines, i + 1);
                if (nextDecl == null)
                {
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: SCRIPT_BIND_PROP() with no field declaration following it.");
                    continue;
                }

                var prop = ParseProperty(nextDecl, PropertyAccessKind.Direct, filePath, i + 1);
                if (prop != null)
                    currentClass.Properties.Add(prop);
                else
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: Could not parse field after SCRIPT_BIND_PROP(). " +
                        $"Found: \"{nextDecl.Trim()}\"");
                continue;
            }

            // -- SCRIPT_BIND_REF_PROP
            if (trimmed.StartsWith(RefPropMarker))
            {
                var nextDecl = FindNextDeclaration(lines, i + 1);
                if (nextDecl == null)
                {
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: SCRIPT_BIND_REF_PROP() with no field declaration following it.");
                    continue;
                }

                var prop = ParseProperty(nextDecl, PropertyAccessKind.RefProp, filePath, i + 1);
                if (prop != null)
                    currentClass.Properties.Add(prop);
                else
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: Could not parse field after SCRIPT_BIND_REF_PROP(). " +
                        $"Found: \"{nextDecl.Trim()}\"");
                continue;
            }

            // -- SCRIPT_BIND_FUNC
            if (trimmed.StartsWith(FuncMarker))
            {
                var nextDecl = FindNextDeclaration(lines, i + 1);
                if (nextDecl == null)
                {
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: SCRIPT_BIND_FUNC() with no function declaration following it.");
                    continue;
                }

                var func = ParseFunction(nextDecl, filePath, i + 1);
                if (func != null)
                    currentClass.Functions.Add(func);
                else
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: Could not parse function after SCRIPT_BIND_FUNC(). " +
                        $"Found: \"{nextDecl.Trim()}\"");
                continue;
            }

            // -- SCRIPT_BIND_FUNC_STATIC
            if (trimmed.StartsWith(StaticFuncMarker))
            {
                var nextDecl = FindNextDeclaration(lines, i + 1);
                if (nextDecl == null)
                {
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: SCRIPT_BIND_FUNC_STATIC() with no function declaration following it.");
                    continue;
                }

                var func = ParseFunction(nextDecl, filePath, i + 1);
                if (func != null)
                {
                    func.IsStatic = true;
                    currentClass.Functions.Add(func);
                }
                else
                    Console.Error.WriteLine(
                        $"[ERROR] {filePath}:{i + 1}: Could not parse function after SCRIPT_BIND_FUNC_STATIC(). " +
                        $"Found: \"{nextDecl.Trim()}\"");
                continue;
            }
        }

        return new ScanResult(classes, enums);
    }

    // ---- Declaration reading ------------------------------------------------

    /// <summary>
    /// Read and concatenate lines starting from startIndex until we have
    /// a complete declaration. A declaration is complete when it contains:
    ///   - Both '(' and ')' (function signature), or
    ///   - A ';' (field/statement), or
    ///   - DECLARE_FIELD with closing ')'
    /// Skips blank lines and comments.
    /// </summary>
    private static string? FindNextDeclaration(string[] lines, int startIndex)
    {
        var sb = new StringBuilder();

        for (int i = startIndex; i < lines.Length; i++)
        {
            var trimmed = lines[i].Trim();
            if (string.IsNullOrEmpty(trimmed)) continue;
            if (trimmed.StartsWith("//")) continue;
            if (trimmed.StartsWith("/*") || trimmed.StartsWith("*")) continue;

            sb.Append(' ');
            sb.Append(trimmed);

            var soFar = sb.ToString();

            if (soFar.Contains('(') && soFar.Contains(')'))
                return soFar;

            if (soFar.Contains(';'))
                return soFar;

            if (soFar.Contains("DECLARE_FIELD") && soFar.Contains(')'))
                return soFar;
        }

        return sb.Length > 0 ? sb.ToString() : null;
    }

    // ---- Parent class extraction --------------------------------------------

    private static List<string> ExtractParentClasses(string classLine)
    {
        var parents = new List<string>();

        int colonIdx = classLine.IndexOf(':');
        if (colonIdx < 0) return parents;

        var inheritancePart = classLine[(colonIdx + 1)..];

        int braceIdx = inheritancePart.IndexOf('{');
        if (braceIdx >= 0)
            inheritancePart = inheritancePart[..braceIdx];

        foreach (var segment in inheritancePart.Split(','))
        {
            var trimmed = segment.Trim();
            if (string.IsNullOrEmpty(trimmed)) continue;

            trimmed = Regex.Replace(trimmed, @"^\s*(public|protected|private)\s+", "");

            int angleIdx = trimmed.IndexOf('<');
            if (angleIdx >= 0)
                trimmed = trimmed[..angleIdx];

            trimmed = trimmed.Trim();
            if (!string.IsNullOrEmpty(trimmed))
                parents.Add(trimmed);
        }

        return parents;
    }

    // ---- Noise stripping ----------------------------------------------------

    private static string StripDeclarationNoise(string line)
    {
        var cleaned = CppAttributePattern.Replace(line, "");
        cleaned = Regex.Replace(cleaned, @"\b(virtual|inline|static|explicit|constexpr)\b", "");
        return cleaned;
    }

    // ---- Property parsing ---------------------------------------------------

    private static BoundProperty? ParseProperty(string line, PropertyAccessKind defaultKind,
                                                 string file, int annotationLine)
    {
        var dfMatch = DeclareFieldPattern.Match(line);
        if (dfMatch.Success)
        {
            return new BoundProperty
            {
                Name = dfMatch.Groups[1].Value,
                CppType = dfMatch.Groups[2].Value.Trim(),
                AccessKind = PropertyAccessKind.DeclareField,
                SourceFile = file,
                Line = annotationLine
            };
        }

        var cleaned = StripDeclarationNoise(line);
        var rawMatch = RawFieldPattern.Match(cleaned);
        if (rawMatch.Success)
        {
            return new BoundProperty
            {
                Name = rawMatch.Groups[2].Value,
                CppType = rawMatch.Groups[1].Value.Trim(),
                AccessKind = defaultKind,
                SourceFile = file,
                Line = annotationLine
            };
        }

        return null;
    }

    // ---- Function parsing ---------------------------------------------------

    private static BoundFunction? ParseFunction(string line, string file, int annotationLine)
    {
        var cleaned = StripDeclarationNoise(line);

        var match = FuncPattern.Match(cleaned);
        if (!match.Success) return null;

        var func = new BoundFunction
        {
            ReturnType = match.Groups[1].Value.Trim(),
            Name = match.Groups[2].Value,
            SourceFile = file,
            Line = annotationLine
        };

        var paramStr = match.Groups[3].Value.Trim();
        if (!string.IsNullOrEmpty(paramStr) && paramStr != "void")
        {
            foreach (var param in paramStr.Split(','))
            {
                var p = param.Trim();
                if (string.IsNullOrEmpty(p)) continue;

                // Strip default values: "EForceMode mode = EForceMode::Force" -> "EForceMode mode"
                int eqIdx = p.IndexOf('=');
                if (eqIdx >= 0)
                    p = p[..eqIdx].Trim();

                var lastSpace = p.LastIndexOf(' ');
                if (lastSpace < 0) continue;

                var pType = p[..lastSpace].Trim();
                var pName = p[(lastSpace + 1)..].Trim();
                func.Parameters.Add((pType, pName));
            }
        }

        return func;
    }

    // ---- Enum parsing -------------------------------------------------------

    /// <summary>
    /// Parse an enum declaration starting from the line after SCRIPT_BIND_ENUM().
    /// Reads the enum name, then scans the body between { } for enumerator names.
    /// Handles: enum class EFoo { A, B = 5, C };
    /// </summary>
    private static BoundEnum? ParseEnum(string[] lines, int startIndex, string file, int annotationLine)
    {
        // Concatenate lines, stripping inline comments per-line so that
        // "Force, // description" does not swallow "Impulse" on the next line
        var sb = new StringBuilder();
        for (int i = startIndex; i < lines.Length; i++)
        {
            var line = lines[i];

            // Strip inline comments before concatenating
            int commentIdx = line.IndexOf("//");
            if (commentIdx >= 0)
                line = line[..commentIdx];

            sb.Append(' ');
            sb.Append(line);

            if (sb.ToString().Contains('}'))
                break;
        }

        var enumText = sb.ToString();

        // Match the enum declaration
        var nameMatch = EnumPattern.Match(enumText);
        if (!nameMatch.Success)
            return null;

        bool isEnumClass = nameMatch.Groups[1].Success;
        string enumName = nameMatch.Groups[2].Value;

        // Extract the body between { and }
        int openBrace = enumText.IndexOf('{');
        int closeBrace = enumText.IndexOf('}');
        if (openBrace < 0 || closeBrace < 0 || closeBrace <= openBrace)
            return null;

        var bodyText = enumText[(openBrace + 1)..closeBrace];

        // Parse individual enumerators
        var values = new List<string>();
        foreach (var entry in bodyText.Split(','))
        {
            var trimmed = entry.Trim();
            if (string.IsNullOrEmpty(trimmed)) continue;

            // Remove comments
            int commentIdx = trimmed.IndexOf("//");
            if (commentIdx >= 0)
                trimmed = trimmed[..commentIdx].Trim();

            // Remove assigned value: "Force = 0" -> "Force"
            int eqIdx = trimmed.IndexOf('=');
            if (eqIdx >= 0)
                trimmed = trimmed[..eqIdx].Trim();

            // Remove any remaining whitespace artifacts
            trimmed = trimmed.Trim();

            if (!string.IsNullOrEmpty(trimmed) && Regex.IsMatch(trimmed, @"^\w+$"))
                values.Add(trimmed);
        }

        if (values.Count == 0)
            return null;

        return new BoundEnum
        {
            Name = enumName,
            IsEnumClass = isEnumClass,
            Values = values,
            SourceFile = file,
            Line = annotationLine
        };
    }
}