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

Move the files in the `exefs` folder (`main.npdm` and `subsdk9`) to the location of your LayeredFS `exefs` folder.

## Usage
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
