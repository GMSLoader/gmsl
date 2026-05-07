# GameMaker Studio Loader

A mod loader for GameMaker Studio 2 games with C# interop and mod blacklist / whitelist support.

## Installation

1. Download the latest release from the [releases page](https://github.com/GMSLoader/gmsl/releases/tag/development-build)
2. Unpack the downloaded archive into your game's root folder

## Usage

### Players

1. Put your mods in `<game root>/gmsl/mods`
2. To block specific mods, add their IDs to `mods/blacklist.txt`
3. To only allow specific mods, add their IDs to `mods/whitelist.txt` — this enables whitelist mode and ignores all other mods

### Modders

Mods are C# assemblies placed in the `mods` folder. The mod API is provided by `gmsl-modapi`.

The final folder structure should look something like this:

```
<game root>
+---...
+---gmsl
|   +---loader
|   |   +---...
|   \---mods
|       +---*your mods here*
|       \---...
+---version.dll
\---...
```

## Compilation

### Prerequisites

- CMake 3.8+
- A C++ compiler with C++17 support (MSVC recommended on Windows)
- .NET SDK (for `gmsl-modapi` and `gmsl-interop`)

### Compile

1. Clone the repo recursively:

    ```sh
    git clone https://github.com/GMSLoader/gmsl.git --recurse-submodules
    cd gmsl
    ```

2. Configure and build with CMake:

    ```sh
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ```

    Output will be placed in the `out/` directory.

### Install

Copy the contents of the `out/` directory into your game's root folder.

## Project Structure

| Directory | Description |
|-----------|-------------|
| `gmsl-loader` | Core loader, loaded into the game process via `version.dll` proxy |
| `gmsl-patcher` | Patches the game's data file at runtime to enable modding hooks |
| `gmsl-interop` | Native bridge that allows GameMaker to call C# |
| `gmsl-modapi` | C# mod API that mods reference to interact with the loader |
| `vendor/` | Third-party dependencies |

## License

Licensed under the [GPL-3.0 License](LICENSE).
