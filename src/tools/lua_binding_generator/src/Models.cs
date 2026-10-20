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

    /// <summary>
    /// Return type and parameters exactly as written in the header. Set by the
    /// scanner and never refined by the AST pass, which can lose template
    /// arguments (std::vector&lt;T&gt; may resolve to plain "vector").
    /// </summary>
    public string DeclaredReturnType { get; set; } = "void";
    public List<(string Type, string Name)> DeclaredParameters { get; set; } = new();

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

    /// <summary>
    /// The value of each enumerator, parallel to Values. Follows C++ rules:
    /// an explicit "= N" sets it, otherwise it is the previous value + 1.
    /// Null for an enumerator whose initialiser is not a plain integer.
    /// </summary>
    public List<long?> NumericValues { get; set; } = new();
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