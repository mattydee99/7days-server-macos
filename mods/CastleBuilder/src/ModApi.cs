namespace CastleBuilder
{
    // Required entry point; the console command is discovered by the game via
    // reflection (any ConsoleCmdAbstract subclass in a mod assembly).
    public class ModApi : IModApi
    {
        public void InitMod(Mod _modInstance)
        {
            Log.Out("[CastleBuilder] Loaded");
        }
    }
}
