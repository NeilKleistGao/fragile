using System.Collections.Generic;
using Godot;

namespace Fragile.Tests;

/// <summary>
/// End-to-end probe for the .NET bindings pipeline (scripts/build_dotnet_bindings.ps1).
/// tests/dotnet_interop.tscn puts this script on an FWorld root node: compiling the
/// file proves compile-time type recognition, running the scene proves runtime
/// recognition of the fragile GDExtension classes from C#. The probe writes
/// res://tests/dotnet_interop_result.txt with one PASS/FAIL line per check and a
/// final DOTNET_INTEROP_PROBE summary line (checked by CI / smoke tests).
/// </summary>
public partial class FWorldProbe : FWorld
{
    public override void _Ready()
    {
        var lines = new List<string>();
        int failed = 0;

        void Check(string name, bool ok, string detail = "")
        {
            string suffix = detail.Length > 0 ? " [" + detail + "]" : "";
            lines.Add((ok ? "PASS " : "FAIL ") + name + suffix);
            if (!ok)
            {
                failed++;
            }
        }

        Check("ClassDB.ClassExists(\"FWorld\")", ClassDB.ClassExists("FWorld"));
        Check("probe node is FWorld (C# can inherit the C++ class)", this is FWorld);
        Check("GetClass() == \"FWorld\"", GetClass() == "FWorld");
        Check("typed GetNode<FWorld>(\".\") resolves (this node)", GetNode<FWorld>(".") != null);
        Check("typed call Serialize(Dictionary) == Error.Ok", Serialize(new Godot.Collections.Dictionary()) == Error.Ok);

        var level = new FLevel();
        Check("typed construction new FLevel()", level.GetClass() == "FLevel",
            "GetClass()=\"" + level.GetClass() + "\" csharpType=" + level.GetType().FullName);
        level.Free();

        GodotObject inst = ClassDB.Instantiate("FWorld").AsGodotObject();
        Check("ClassDB.Instantiate(\"FWorld\") wraps as typed FWorld", inst is FWorld,
            inst == null ? "null" : "GetClass()=\"" + inst.GetClass() + "\" csharpType=" + inst.GetType().FullName);
        inst?.Free();

        lines.Add(failed == 0 ? "DOTNET_INTEROP_PROBE: PASS" : "DOTNET_INTEROP_PROBE: FAIL (" + failed + " checks failed)");
        GD.Print(string.Join("\n", lines));

        using var file = FileAccess.Open("res://tests/dotnet_interop_result.txt", FileAccess.ModeFlags.Write);
        file?.StoreString(string.Join("\n", lines) + "\n");
    }
}
