# wonder-sdfs

File caching bypasses for Super Mario Bros. Wonder

## Features
- [X] Force-reload files in the `BancMapUnit/` folder
- [ ] Force-reload any `.bgyml` file~~
- [X] Replace all baked level collisions (`.bphsc`) with a single file.

## Supported Versions
- 1.0.0
- 1.0.1
- 1.0.2
- 1.2.1

## Notes and Limitations
The replacement method is case-by-case; not all files can be supported. Files that only need to be loaded once can be put into your normal LayeredFS `romfs` folder. Files that need to be repeatedly reloaded should use this patch.

Due to using `malloc` and not the `sead` heaps, the size limitations on files externally loaded should, in theory, no longer matter.

## Included Files
- `BLANK.bphsc.zs`

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

# If you want to use BLANK.bphsc.zs, enable this option.
USE_UNIVERSAL_BLANK_BPHSC="true"
```

If using any of the assets included in this patch, create a folder called `wonder-sdfs-assets/` inside the`wonder-sdfs/` folder and include them there.

Your file tree should look like this:
```
📁 [SD ROOT]
└── 📁 wonder-sdfs/
    ├── 📁 wonder-sdfs-assets/
    │    └── 📄 BLANK.bphsc.zs
    ├── 📄 config.cfg
    └── 📁 YourFolderName/
```

## Dependencies
- [zstd](https://github.com/meta/zstd)
