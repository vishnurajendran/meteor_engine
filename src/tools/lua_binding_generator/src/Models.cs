// Models.cs

using System.Collections.Generic;

namespace MeteorBindingGenerator;

public enum PropertyAccessKind
{
    Direct,
    DeclareField,
    RefProp
}

public class BoundProperty
{
    public string Name { get; set; } = "";
    public string CppType { get; set; } = "";
    public PropertyAccessKind AccessKind { get; set; } = PropertyAccessKind.Direct;
    public string SourceFile { get; set; } = "";
    public int Line { get; set; }
}

public class BoundFunction
{
    public string Name { get; set; } = "";
    public string ReturnType { get; set; } = "void";
    public List<(string Type, string Name)> Parameters { get; set; } = new();
    public string SourceFile { get; set; } = "";
    public int Line { get; set; }

    /// <summary>Whether this is a static member function (SCRIPT_BIND_FUNC_STATIC).</summary>
    public bool IsStatic { get; set; } = false;
}

public class BoundEnum
{
    public string Name { get; set; } = "";
    public bool IsEnumClass { get; set; } = false;
    public List<string> Values { get; set; } = new();
    public string SourceFile { get; set; } = "";
    public int Line { get; set; }
}

public class BoundClass
{
    public string Name { get; set; } = "";
    public string SourceFile { get; set; } = "";
    public List<string> ParentClasses { get; set; } = new();
    public List<BoundProperty> Properties { get; set; } = new();
    public List<BoundFunction> Functions { get; set; } = new();
}