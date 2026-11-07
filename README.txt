Template project for code modification with Hakkun. (click 'Use this template' to create your own repository)
Read through the TODO list and tick off individual tasks as you set up the repository for your specific target.

--- TODO ---

[X] name the project (project() call in CMakeLists.txt and MODULE_NAME in config/config.cmake)

Target configuration:
[X] config.cmake: Set TITLE_ID to your title's ID
[X] config.cmake: Set TARGET_IS_STATIC to whether or not your target statically links rtld into its main executable (sysmodules do this, applications do not)
[X] config.cmake: Set SDK_PAST_1900 to whether or not your target uses nnSdk 19.0.0 or newer (titles released or updated in late 2024 or later)
[X] config.cmake: Set IS_32_BIT if your target is 32 bit
[X] Set up config/npdm.json:
    Extract the main.npdm from your target, convert it to json using hactool and place it in config/npdm.json
    Since hactool misses the system_resource_size in conversion, open the target's main.npdm in a hex editor, copy the u32 value at 0x14 out and add it to the generated json in this format:
        "system_resource_size": "0xVALUE",
    Then modify the json [as described here](https://codeberg.org/fruityloops1/LibHakkun/src/branch/main/README.md#npdm-json).

Setting up sail:
[X] config.cmake: Set USE_SAIL:
    If you want to dynamic link to symbols that you can source using offsets, dynamic symbol lookup, data blocks and more with support for multiple versions at once, enable sail.
    If you wish to handle linking another way, disable it and skip the rest of the steps related to sail.
[X] config.cmake: Set BAKE_SYMBOLS:
    Enabling this will import sail symbols by hash instead of string (smaller when compressed, more obfuscated).
[X] Set up config/VersionList.sym:
    Add all of the modules you'd like to link to using sail to this file in this format:
        @myModuleName = PathName
    PathName being the (or a part of the) mod0 path name of the module (this can be found at the start of the rodata section, usually ending in .nss)
    To add multiple versions for a module, add them after the module line in this format:hhh
        versionName = BuildId
    Where BuildId is the build ID of the NSO (found at 0x40 in the NSO header or after the 'GNU\0' string in rodata)
Features:
[X] config.cmake: Set TRAMPOLINE_LEVEL:
    The trampoline level sets how complex of hook sites of your trampoline hooks can be installed at. Higher = more text memory usage per hook.
    * 0: only basic backups (no b.cond, cbz/cbnz, tbz/tbnz), +-128MB branch range
    * 1: all backups, +-128MB branch range
    * 2: all backups, infinite branch range
[X] config.cmake: Set USE_ADVANCED_RESULT:
    When enabled, hk::Result will keep a stack trace of results with the locations of their construction in it, useful for tracing origins of failed results if you have logging enabled
[X] config.cmake: Add any addons you'd like to use to HAKKUN_ADDONS (see [Hakkun README](https://codeberg.org/fruityloops1/LibHakkun/src/branch/main/README.md#features) for a list of addons)

[X] Setup GitHub/Forgejo action:
    If you would like to use actions to build your project on push, copy the workflow template from the Hakkun submodule in sys/.github/workflow_templates into your own project's .github/workflows or .forgejo/workflows folder.

When you are done ticking off tasks, try building and remove this file when you're done.
