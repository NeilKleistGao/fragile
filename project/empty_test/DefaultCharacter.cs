using Godot;

public partial class DefaultCharacter : FCharacter {
  private void Quit(bool started, bool ongoing, bool completed) {
    GD.Print($"ui: {started}, {ongoing}, {completed}");
    if (completed) {
      GetTree().Quit(); 
    }
  }

  public override void _SetupInputComponent(FInputComponent ic) {
    base._SetupInputComponent(ic);
    ic.BindAction("ui_accept", Callable.From<bool, bool, bool>(Quit));
  }
}
