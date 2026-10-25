using System.Collections.Generic;
using System.IO;
using System.Text;
using UnityEngine;

namespace CastleBuilder
{
    public class ConsoleCmdCastle : ConsoleCmdAbstract
    {
        // Chunk observers we've added, keyed by "x,z", so they can be removed later.
        static readonly Dictionary<string, ChunkManager.ChunkObserver> observers =
            new Dictionary<string, ChunkManager.ChunkObserver>();

        public override string[] getCommands()
        {
            return new[] { "castle" };
        }

        public override string getDescription()
        {
            return "Survey terrain and place generated structures";
        }

        public override string getHelp()
        {
            return "Usage:\n" +
                   "  castle load <x> <z> [radius]   - keep chunks around x,z loaded (radius in chunks, 1-15, default 3)\n" +
                   "  castle unload <x> <z>          - remove a chunk observer added by 'castle load'\n" +
                   "  castle survey <x1> <z1> <x2> <z2> - print ground y, top y and top block for each column\n" +
                   "  castle scan <x1> <z1> <x2> <z2>  - list what 'castle strip' would remove (dry run)\n" +
                   "  castle strip <x1> <z1> <x2> <z2> - remove trees, plants, rocks and other non-terrain blocks above ground\n" +
                   "  castle build <plan> <name>      - apply /home/steam/castle/<plan> ('fill x1 y1 z1 x2 y2 z2 block [rot]' lines), saving an undo log as <name>\n" +
                   "  castle undo <name>              - restore every block changed by 'castle build ... <name>'\n" +
                   "  castle poc <x> <z>             - place a 3x3x3 steel cube on the ground centred on x,z\n" +
                   "  castle clear <x> <z>           - remove the 3x3x3 cube placed by 'castle poc'";
        }

        static void Out(string _s)
        {
            SingletonMonoBehaviour<SdtdConsole>.Instance.Output(_s);
        }

        public override void Execute(List<string> _params, CommandSenderInfo _senderInfo)
        {
            if (_params.Count == 0)
            {
                Out(getHelp());
                return;
            }

            string sub = _params[0].ToLowerInvariant();
            if (sub == "build" && _params.Count == 3)
            {
                Build(_params[1], _params[2]);
                return;
            }
            if (sub == "undo" && _params.Count == 2)
            {
                Undo(_params[1]);
                return;
            }

            var ints = new List<int>();
            for (int i = 1; i < _params.Count; i++)
            {
                if (!int.TryParse(_params[i], out var v))
                {
                    Out("Not an integer: " + _params[i]);
                    return;
                }
                ints.Add(v);
            }

            switch (_params[0].ToLowerInvariant())
            {
                case "load" when ints.Count == 2 || ints.Count == 3:
                    Load(ints[0], ints[1], ints.Count == 3 ? ints[2] : 3);
                    break;
                case "unload" when ints.Count == 2:
                    Unload(ints[0], ints[1]);
                    break;
                case "survey" when ints.Count == 4:
                    Survey(ints[0], ints[1], ints[2], ints[3]);
                    break;
                case "scan" when ints.Count == 4:
                    Strip(ints[0], ints[1], ints[2], ints[3], false);
                    break;
                case "strip" when ints.Count == 4:
                    Strip(ints[0], ints[1], ints[2], ints[3], true);
                    break;
                case "poc" when ints.Count == 2:
                    Cube(ints[0], ints[1], Block.GetBlockValue("steelShapes:cube"));
                    break;
                case "clear" when ints.Count == 2:
                    Cube(ints[0], ints[1], BlockValue.Air);
                    break;
                default:
                    Out(getHelp());
                    break;
            }
        }

        static void Load(int _x, int _z, int _radius)
        {
            string key = _x + "," + _z;
            if (observers.ContainsKey(key))
            {
                Out("Already observing " + key);
                return;
            }
            _radius = Mathf.Clamp(_radius, 1, 15);
            observers[key] = GameManager.Instance.AddChunkObserver(new Vector3(_x, 0f, _z), false, _radius, -1);
            Out("Chunk observer added at " + key + " radius " + _radius + "; chunks load over the next few seconds");
        }

        static void Unload(int _x, int _z)
        {
            string key = _x + "," + _z;
            if (!observers.TryGetValue(key, out var obs))
            {
                Out("No observer at " + key);
                return;
            }
            GameManager.Instance.RemoveChunkObserver(obs);
            observers.Remove(key);
            Out("Chunk observer removed at " + key);
        }

        static bool Loaded(World _world, int _x, int _z)
        {
            return _world.GetChunkFromWorldPos(_x, 0, _z) != null;
        }

        // Highest block in the column that isn't air, a plant or other
        // walk-through decoration (grass sits one block above the ground).
        static int GroundY(World _world, int _x, int _z)
        {
            int y = _world.GetHeight(_x, _z);
            while (y > 0)
            {
                Block b = _world.GetBlock(_x, y, _z).Block;
                if (!b.IsPlant() && !b.IsTerrainDecoration && b.IsCollideMovement)
                    break;
                y--;
            }
            return y;
        }

        // One line per column: "x z ground top blockName". The top block is the
        // highest non-air block, which may be a tree, grass or a player block.
        static void Survey(int _x1, int _z1, int _x2, int _z2)
        {
            World world = GameManager.Instance.World;
            int xa = Mathf.Min(_x1, _x2), xb = Mathf.Max(_x1, _x2);
            int za = Mathf.Min(_z1, _z2), zb = Mathf.Max(_z1, _z2);
            if ((xb - xa + 1) * (zb - za + 1) > 10000)
            {
                Out("Area too large (max 10000 columns)");
                return;
            }

            var sb = new StringBuilder();
            int missing = 0;
            sb.AppendLine("SURVEY_BEGIN " + xa + " " + za + " " + xb + " " + zb);
            for (int z = za; z <= zb; z++)
            {
                for (int x = xa; x <= xb; x++)
                {
                    if (!Loaded(world, x, z))
                    {
                        missing++;
                        continue;
                    }
                    int h = world.GetHeight(x, z);
                    BlockValue bv = world.GetBlock(x, h, z);
                    sb.Append(x).Append(' ').Append(z).Append(' ').Append(GroundY(world, x, z)).Append(' ')
                      .Append(h).Append(' ')
                      .AppendLine(bv.Block.GetBlockName());
                }
            }
            sb.Append("SURVEY_END missing=").Append(missing);
            Out(sb.ToString());
        }

        // Every non-air block above the terrain surface, except water and our own
        // steel. Multi-block pieces (trees, big rocks) are resolved to their
        // parent so removing it takes the whole thing. Lists counts per block
        // name and, for multi-blocks, their parent positions.
        static void Strip(int _x1, int _z1, int _x2, int _z2, bool _remove)
        {
            World world = GameManager.Instance.World;
            int xa = Mathf.Min(_x1, _x2), xb = Mathf.Max(_x1, _x2);
            int za = Mathf.Min(_z1, _z2), zb = Mathf.Max(_z1, _z2);
            if ((xb - xa + 1) * (zb - za + 1) > 10000)
            {
                Out("Area too large (max 10000 columns)");
                return;
            }

            var targets = new HashSet<Vector3i>();
            int missing = 0;
            for (int z = za; z <= zb; z++)
            for (int x = xa; x <= xb; x++)
            {
                if (!Loaded(world, x, z))
                {
                    missing++;
                    continue;
                }
                for (int y = 254; y > 0; y--)
                {
                    var pos = new Vector3i(x, y, z);
                    BlockValue bv = world.GetBlock(pos);
                    if (bv.isair)
                        continue;
                    if (bv.Block.shape.IsTerrain())
                        break;
                    if (bv.ischild)
                    {
                        pos += bv.parent;
                        bv = world.GetBlock(pos);
                    }
                    if (bv.isair || bv.isWater || bv.Block.GetBlockName().StartsWith("steel"))
                        continue;
                    targets.Add(pos);
                }
            }

            var counts = new SortedDictionary<string, int>();
            var sb = new StringBuilder();
            var changes = new List<BlockChangeInfo>();
            sb.AppendLine((_remove ? "STRIP_BEGIN " : "SCAN_BEGIN ") + xa + " " + za + " " + xb + " " + zb);
            foreach (Vector3i pos in targets)
            {
                BlockValue bv = world.GetBlock(pos);
                string name = bv.Block.GetBlockName();
                counts.TryGetValue(name, out int n);
                counts[name] = n + 1;
                if (bv.Block.isMultiBlock)
                    sb.Append("multi ").Append(pos.x).Append(' ').Append(pos.y).Append(' ').Append(pos.z).Append(' ').AppendLine(name);
                if (_remove)
                    changes.Add(new BlockChangeInfo(new BlockValueRef(pos), BlockValue.Air, MarchingCubes.DensityAir));
            }
            foreach (var kv in counts)
                sb.Append("count ").Append(kv.Value).Append(' ').AppendLine(kv.Key);
            if (_remove && changes.Count > 0)
                world.SetBlocksRPC(changes);
            sb.Append(_remove ? "STRIP_END removed=" : "SCAN_END found=").Append(targets.Count).Append(" missing=").Append(missing);
            Out(sb.ToString());
        }

        const string Dir = "/home/steam/castle/";

        static bool SafeName(string _s)
        {
            foreach (char c in _s)
                if (!char.IsLetterOrDigit(c) && c != '-' && c != '_' && c != '.')
                    return false;
            return _s.Length > 0 && !_s.StartsWith(".");
        }

        static sbyte DensityFor(BlockValue _bv)
        {
            return !_bv.isair && _bv.Block.shape.IsTerrain() ? MarchingCubes.DensityTerrain : MarchingCubes.DensityAir;
        }

        static void Apply(World _world, List<BlockChangeInfo> _changes)
        {
            // Send in batches so no single network package gets huge.
            for (int i = 0; i < _changes.Count; i += 2000)
                _world.SetBlocksRPC(_changes.GetRange(i, Mathf.Min(2000, _changes.Count - i)));
        }

        // Later fill lines override earlier ones at the same position, so a
        // plan can carve openings (fill ... air) after placing solid walls.
        static void Build(string _plan, string _name)
        {
            if (!SafeName(_plan) || !SafeName(_name))
            {
                Out("Plan and undo names may only contain letters, digits, '-', '_' and '.'");
                return;
            }
            string undoPath = Dir + "undo-" + _name + ".txt";
            if (File.Exists(undoPath))
            {
                Out("Undo log " + _name + " already exists; pick another name or 'castle undo " + _name + "' first");
                return;
            }
            string[] lines;
            try { lines = File.ReadAllLines(Dir + _plan); }
            catch (IOException e) { Out("Cannot read plan: " + e.Message); return; }

            World world = GameManager.Instance.World;
            var cache = new Dictionary<string, BlockValue>();
            var blocks = new Dictionary<Vector3i, BlockValue>();
            var order = new List<Vector3i>();
            for (int n = 0; n < lines.Length; n++)
            {
                string line = lines[n].Trim();
                if (line.Length == 0 || line.StartsWith("#"))
                    continue;
                string[] f = line.Split((char[])null, System.StringSplitOptions.RemoveEmptyEntries);
                var c = new int[6];
                bool ok = f[0] == "fill" && (f.Length == 8 || f.Length == 9);
                for (int i = 0; ok && i < 6; i++)
                    ok = int.TryParse(f[i + 1], out c[i]);
                int rot = 0;
                if (ok && f.Length == 9)
                    ok = int.TryParse(f[8], out rot) && rot >= 0 && rot < 32;
                if (!ok)
                {
                    Out("Plan line " + (n + 1) + " is malformed: " + line);
                    return;
                }
                if (!cache.TryGetValue(f[7], out BlockValue bv))
                {
                    bv = f[7] == "air" ? BlockValue.Air : Block.GetBlockValue(f[7]);
                    if (!bv.isair && bv.type == 0)
                    {
                        Out("Plan line " + (n + 1) + ": unknown block " + f[7]);
                        return;
                    }
                    cache[f[7]] = bv;
                }
                bv.rotation = (byte)rot;
                for (int y = Mathf.Min(c[1], c[4]); y <= Mathf.Max(c[1], c[4]); y++)
                for (int z = Mathf.Min(c[2], c[5]); z <= Mathf.Max(c[2], c[5]); z++)
                for (int x = Mathf.Min(c[0], c[3]); x <= Mathf.Max(c[0], c[3]); x++)
                {
                    var pos = new Vector3i(x, y, z);
                    if (y < 1 || y > 254)
                    {
                        Out("Plan line " + (n + 1) + ": y out of range");
                        return;
                    }
                    if (!blocks.ContainsKey(pos))
                        order.Add(pos);
                    blocks[pos] = bv;
                }
            }
            foreach (Vector3i pos in order)
            {
                if (!Loaded(world, pos.x, pos.z))
                {
                    Out("Chunk at " + pos.x + "," + pos.z + " is not loaded; run 'castle load' first");
                    return;
                }
            }

            var undo = new StringBuilder();
            var changes = new List<BlockChangeInfo>();
            foreach (Vector3i pos in order)
            {
                BlockValue old = world.GetBlock(pos);
                BlockValue bv = blocks[pos];
                undo.Append(pos.x).Append(' ').Append(pos.y).Append(' ').Append(pos.z).Append(' ')
                    .Append(old.rawData).Append(' ').Append(world.GetDensity(pos.x, pos.y, pos.z)).Append('\n');
                changes.Add(new BlockChangeInfo(new BlockValueRef(pos), bv, DensityFor(bv)));
            }
            File.WriteAllText(undoPath, undo.ToString());
            Apply(world, changes);
            Out("Built " + changes.Count + " blocks from " + _plan + "; undo with 'castle undo " + _name + "'");
        }

        static void Undo(string _name)
        {
            if (!SafeName(_name))
            {
                Out("Bad name");
                return;
            }
            string undoPath = Dir + "undo-" + _name + ".txt";
            string[] lines;
            try { lines = File.ReadAllLines(undoPath); }
            catch (IOException e) { Out("Cannot read undo log: " + e.Message); return; }

            World world = GameManager.Instance.World;
            var changes = new List<BlockChangeInfo>();
            foreach (string line in lines)
            {
                string[] f = line.Split(' ');
                if (f.Length != 5)
                    continue;
                var pos = new Vector3i(int.Parse(f[0]), int.Parse(f[1]), int.Parse(f[2]));
                if (!Loaded(world, pos.x, pos.z))
                {
                    Out("Chunk at " + pos.x + "," + pos.z + " is not loaded; run 'castle load' first");
                    return;
                }
                changes.Add(new BlockChangeInfo(new BlockValueRef(pos), new BlockValue(uint.Parse(f[3])), sbyte.Parse(f[4])));
            }
            Apply(world, changes);
            File.Move(undoPath, undoPath + ".done");
            Out("Restored " + changes.Count + " blocks from undo log " + _name);
        }

        // 3x3x3 cube whose bottom layer sits directly on the ground at the
        // centre column, replacing any grass. Placing Air instead removes it
        // again (the cube is then the top of the column).
        static void Cube(int _x, int _z, BlockValue _bv)
        {
            World world = GameManager.Instance.World;
            if (!Loaded(world, _x, _z))
            {
                Out("Chunk at " + _x + "," + _z + " is not loaded; run 'castle load " + _x + " " + _z + "' first");
                return;
            }
            if (!_bv.isair && _bv.type == 0)
            {
                Out("Block not found");
                return;
            }

            bool placing = !_bv.isair;
            int baseY = placing ? GroundY(world, _x, _z) + 1 : world.GetHeight(_x, _z) - 2;
            var changes = new List<BlockChangeInfo>();
            for (int dy = 0; dy < 3; dy++)
            for (int dx = -1; dx <= 1; dx++)
            for (int dz = -1; dz <= 1; dz++)
            {
                var pos = new Vector3i(_x + dx, baseY + dy, _z + dz);
                changes.Add(new BlockChangeInfo(new BlockValueRef(pos), _bv, MarchingCubes.DensityAir));
            }
            world.SetBlocksRPC(changes);
            Out((placing ? "Placed " : "Cleared ") + changes.Count + " blocks at x=" + (_x - 1) + ".." + (_x + 1) +
                " y=" + baseY + ".." + (baseY + 2) + " z=" + (_z - 1) + ".." + (_z + 1));
        }
    }
}
