# wonder-sdfs

## Features
Reload the following files:

- [x] `BancMapUnit/*`
- [ ] `Gyml/Gfx/EnvPaletteParam/*`
- [ ] `Stage/AreaParam/*`
- [ ] `Stage/StageLoadInfo/*`
- [ ] `Stage/MapAnalysisInfo/*`

## Supported Versions
- 1.0.0

## Notes and Limitations
The replacement method is case-by-case; not all files can be supported. Files that only need to be loaded once can be put into your normal LayeredFS `romfs` folder. Files that need to be repeatedly reloaded should use this patch.

This patch simply loads the data and replaces at runtime; it does not change how much data is allocated for each file. If you intend to increase the size of certain files while the game is running, preallocate space in the game's size table beforehand.

## Building and Usage
*Recursively* clone the repository:
```
git clone https://github.com/Swiftshine/wonder-sdfs --recursive
```

Run:
```
cmake -B build
cmake --build build
```

Move the files in the `build/exefs` folder (`main.npdm` and `subsdk9`) to the location of your LayeredFS `exefs` folder.

Create a folder on the root of your SD card named `wonder-sdfs`. Inside that folder, create a text file called `config.cfg` and set it up as such:
```toml
# This is where the romfs of your mod will go.
# This must not exceed 255 characters.
# Do not put a trailing '/' character.
PROJECT_ROMFS_FOLDER="YourFolderName"
```

Your file tree should look like this:
```
[SD ROOT]
    └── [FOLDER] wonder-sdfs
        ├── [FILE] config.cfg
        └── [FOLDER] YourFolderName/
```
## Dependencies
- [zstd](https://github.com/meta/zstd)
