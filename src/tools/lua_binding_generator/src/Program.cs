// Program.cs
// Entry point for MeteorBindingGenerator.

using System;
using System.IO;

namespace MeteorBindingGenerator;

public static class Program
{
    public static int Main(string[] args)
    {
        string? sourceDir = null;
        string? outputDir = null;
        string? stubDir   = null;

        for (int i = 0; i < args.Length; i++)
        {
            switch (args[i])
            {
                case "--source-dir" when i + 1 < args.Length:
                    sourceDir = args[++i];
                    break;
                case "--output-dir" when i + 1 < args.Length:
                    outputDir = args[++i];
                    break;
                case "--stub-dir" when i + 1 < args.Length:
                    stubDir = args[++i];
                    break;
            }
        }

        if (sourceDir == null || outputDir == null || stubDir == null)
        {
            Console.Error.WriteLine("Usage: MeteorBindingGenerator");
            Console.Error.WriteLine("  --source-dir <path>   Root directory to scan for annotated headers");
            Console.Error.WriteLine("  --output-dir <path>   Directory for generated .binding.h files");
            Console.Error.WriteLine("  --stub-dir   <path>   Directory containing stub headers for CppAst");
            return 2;
        }

        if (!Directory.Exists(sourceDir))
        {
            Console.Error.WriteLine($"[ERROR] Source directory does not exist: {sourceDir}");
            return 2;
        }

        if (!Directory.Exists(stubDir))
        {
            Console.Error.WriteLine($"[ERROR] Stub directory does not exist: {stubDir}");
            return 2;
        }

        Console.WriteLine("=== MeteorBindingGenerator ===");
        Console.WriteLine($"  Source: {sourceDir}");
        Console.WriteLine($"  Output: {outputDir}");
        Console.WriteLine($"  Stubs:  {stubDir}");
        Console.WriteLine();

        // ---- Pass 1: Text scan for annotations ----
        Console.WriteLine("[Pass 1] Scanning for SCRIPT_BIND annotations...");
        var scanResult = AnnotationScanner.ScanDirectory(sourceDir);
        var boundClasses = scanResult.Classes;
        var boundEnums = scanResult.Enums;

        if (boundClasses.Count == 0 && boundEnums.Count == 0)
        {
            Console.WriteLine("  No SCRIPT_BIND annotations found. Nothing to generate.");
            return 0;
        }

        if (boundClasses.Count > 0)
        {
            Console.WriteLine($"  Found {boundClasses.Count} bound class(es):");
            foreach (var cls in boundClasses)
            {
                var parents = cls.ParentClasses.Count > 0
                    ? $" : {string.Join(", ", cls.ParentClasses)}"
                    : "";
                Console.WriteLine($"    {cls.Name}{parents}  ({cls.Properties.Count} props, {cls.Functions.Count} funcs)");
            }
        }

        if (boundEnums.Count > 0)
        {
            Console.WriteLine($"  Found {boundEnums.Count} bound enum(s):");
            foreach (var e in boundEnums)
                Console.WriteLine($"    {e.Name}  ({e.Values.Count} values: {string.Join(", ", e.Values)})");
        }

        Console.WriteLine();

        // ---- Pass 2a: AST type resolution ----
        Console.WriteLine("[Pass 2a] Resolving types with CppAst...");
        AstTypeResolver.ResolveTypes(boundClasses, sourceDir, stubDir);
        Console.WriteLine();

        // ---- Pass 2b: Validation ----
        Console.WriteLine("[Pass 2b] Validating cross-references...");
        int errorCount = BindingValidator.Validate(boundClasses);

        if (errorCount > 0)
        {
            Console.Error.WriteLine();
            Console.Error.WriteLine($"[FAILED] {errorCount} validation error(s). Build stopped.");
            Console.Error.WriteLine("Fix the errors above and rebuild.");
            return 1;
        }

        Console.WriteLine("  All validations passed.");
        Console.WriteLine();

        // ---- Pass 3: Code generation ----
        Console.WriteLine("[Pass 3] Generating binding code...");
        CodeEmitter.EmitAll(boundClasses, boundEnums, outputDir, sourceDir);
        RegistryEmitter.Emit(boundClasses, boundEnums, outputDir);
        LuaTypeDefEmitter.Emit(boundClasses, boundEnums, outputDir);
        Console.WriteLine();

        int totalTypes = boundClasses.Count + boundEnums.Count;
        Console.WriteLine($"[DONE] Generated bindings for {totalTypes} type(s) " +
                          $"({boundClasses.Count} classes, {boundEnums.Count} enums) in {outputDir}");
        return 0;
    }
}