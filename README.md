# Classes and Plugins for SuperCollider
Collection of Classes and Plugins for SuperCollider 3.14
# Class and Plugin for SuperCollider

**A collection of SuperCollider classes, language-side tools, and native audio plugins for sound synthesis, signal processing, algorithmic composition, and musical experimentation.**

This repository preserves several generations of the HP class and plugin collection. The current repository includes packaged class/plugin builds, C++ plugin sources, and a legacy package prepared for SuperCollider 3.10.

> **Project status: experimental / legacy collection**
>
> The repository currently contains archive packages rather than an unpacked source tree. Read the compatibility, installation, and safety notes before installing anything into an active SuperCollider setup.

---

## Table of contents

- [Repository contents](#repository-contents)
- [Which package should I use?](#which-package-should-i-use)
- [Requirements](#requirements)
- [Installation](#installation)
- [Recommended repository structure](#recommended-repository-structure)
- [Class installation](#class-installation)
- [Plugin installation](#plugin-installation)
- [Building the C++ plugins](#building-the-c-plugins)
- [Verification](#verification)
- [Using the classes and plugins](#using-the-classes-and-plugins)
- [Compatibility](#compatibility)
- [Troubleshooting](#troubleshooting)
- [Development and contributions](#development-and-contributions)
- [Release process](#release-process)
- [License](#license)
- [Acknowledgements](#acknowledgements)

---

## Repository contents

The repository currently exposes the following archives:

| Archive | Intended content | Recommended use |
|---|---|---|
| `HP Class and Plugin.zip` | Packaged HP classes and compiled plugins | End users who need a ready-to-install package, after checking compatibility |
| `HP Plugins C++.zip` | C++ source code for the native plugins | Developers who want to inspect or rebuild the plugins |
| `HP Plugins+Class pour SC 3.10.zip` | Legacy classes and plugins prepared for SuperCollider 3.10 | Archival use or an explicitly maintained SuperCollider 3.10 installation |

The repository page currently shows these three archives directly in the main branch. A future release should move downloadable binaries to GitHub Releases and expose the source tree directly in `Classes/`, `plugins/`, `HelpSource/`, and `Examples/` folders.

---

## Which package should I use?

### I use a recent SuperCollider version

Start with:

```text
HP Class and Plugin.zip
```

Before installation, verify:

- the SuperCollider version used to build the package;
- the operating system;
- CPU architecture;
- whether the native plugins are signed on macOS;
- whether the plugins are Universal 2, Apple Silicon, Intel, Windows, or Linux binaries.

Do not assume that a plugin compiled for SuperCollider 3.10 or Intel macOS will load in SuperCollider 3.14 on Apple Silicon.

### I want to rebuild the native plugins

Use:

```text
HP Plugins C++.zip
```

You will need a compatible SuperCollider source tree, CMake, a C++ compiler, and the development tools for the target operating system.

### I explicitly use SuperCollider 3.10

Use the legacy archive:

```text
HP Plugins+Class pour SC 3.10.zip
```

Keep this package isolated from modern installations. Do not mix 3.10 class files and plugins into a SuperCollider 3.14 extension folder without first reviewing and updating the source.

---

## Requirements

Requirements depend on whether you use language-side classes, compiled plugins, or C++ sources.

### Language-side classes

- SuperCollider
- A writable user extensions directory
- Any Quarks or third-party classes explicitly named in the class documentation

Find the user extensions directory with:

```supercollider
Platform.userExtensionDir.postln;
```

Typical macOS location:

```text
~/Library/Application Support/SuperCollider/Extensions/
```

### Precompiled native plugins

- The exact SuperCollider plugin ABI expected by the archive
- A compatible operating system and CPU architecture
- A plugin binary built for the installed SuperCollider version

### Building C++ plugins

Recommended baseline:

```text
CMake:                 recent stable version
Compiler on macOS:     Apple Clang from Xcode Command Line Tools
Compiler on Linux:     GCC or Clang
Compiler on Windows:   Visual Studio / MSVC supported by the target SC build
SuperCollider sources: version matching the intended plugin target
```

The precise requirements should be recorded in each versioned release.

---

## Installation

### Important: back up the existing SuperCollider environment

Before installing:

1. Quit SuperCollider.
2. Back up the current `Extensions` folder.
3. Search for older HP classes and plugins.
4. Remove or disable duplicate class definitions.
5. Keep legacy packages in a separate archive folder.

A duplicate class name can prevent the entire SuperCollider class library from compiling.

### Recommended installation layout

```text
Platform.userExtensionDir/
└── HP-Class-and-Plugin/
    ├── Classes/
    ├── HelpSource/
    │   └── Classes/
    ├── plugins/
    ├── Examples/
    ├── README.md
    └── LICENSE
```

The exact plugin folder may vary by package and platform. Preserve the folder structure supplied by a verified release.

---

## Recommended repository structure

The repository would be easier to inspect, test, and contribute to with this layout:

```text
Class-and-Plugin-for-SuperCollider/
├── README.md
├── CHANGELOG.md
├── CONTRIBUTING.md
├── LICENSE
├── CMakeLists.txt
├── Classes/
│   └── HPExample.sc
├── HelpSource/
│   ├── Classes/
│   │   └── HPExample.schelp
│   └── Guides/
├── plugins/
│   ├── HPExample/
│   │   ├── HPExample.cpp
│   │   └── CMakeLists.txt
│   └── ...
├── Examples/
│   ├── QuickStart.scd
│   └── PluginTests.scd
├── Tests/
├── Documentation/
└── .github/
    ├── ISSUE_TEMPLATE/
    └── workflows/
```

Compiled archives should be attached to GitHub Releases instead of stored permanently in the main branch.

---

## Class installation

1. Extract the selected archive.
2. Locate the `.sc` class files.
3. Copy the HP extension folder into:

   ```supercollider
   Platform.userExtensionDir;
   ```

4. Start SuperCollider.
5. Select:

   ```text
   Language > Recompile Class Library
   ```

6. Read the Post Window carefully.

### Detecting duplicate classes

If SuperCollider reports that a class is already defined:

1. Copy the class name from the error.
2. Search every configured extension path.
3. Keep only the intended version.
4. Recompile the class library.
5. Create new instances after recompilation.

Inspect extension paths with:

```supercollider
LanguageConfig.includePaths.postln;
LanguageConfig.excludePaths.postln;
Platform.userExtensionDir.postln;
```

---

## Plugin installation

A SuperCollider server plugin is a compiled native library loaded by `scsynth` or `supernova`. Language-side `.sc` files alone are not sufficient for a native UGen.

### General procedure

1. Confirm the plugin matches the operating system and architecture.
2. Confirm the plugin was built for a compatible SuperCollider version.
3. Quit SuperCollider.
4. Copy the complete extension folder, including plugin binaries and class files, to the user extensions directory.
5. Restart SuperCollider.
6. Recompile the class library.
7. Boot the audio server.
8. Run the smallest available plugin example.

### macOS architecture

Check the architecture of a plugin binary in Terminal:

```bash
file /path/to/Plugin.scx
```

For a Universal 2 binary, the output should indicate both:

```text
x86_64
arm64
```

A plugin compiled only for `x86_64` may not load in a native Apple Silicon SuperCollider process.

### macOS Gatekeeper and quarantine

If macOS blocks a downloaded plugin, first verify its release, checksum, and origin. Do not disable Gatekeeper globally.

If a trusted release is quarantined, document the exact remediation in the release notes. Any command that removes quarantine attributes should be presented as an advanced, explicit action rather than an automatic installation step.

---

## Building the C++ plugins

The `HP Plugins C++.zip` archive is intended for developers. The repository should eventually expose these files directly, rather than only inside a ZIP archive.

### 1. Install build tools on macOS

```bash
xcode-select --install
```

Install CMake, for example with Homebrew:

```bash
brew install cmake
```

### 2. Obtain matching SuperCollider sources

Use a SuperCollider source version compatible with the intended plugin target. For a SuperCollider 3.14 plugin, use the corresponding 3.14 source branch or tag when possible.

### 3. Configure an out-of-tree build

A typical structure is:

```text
project/
├── CMakeLists.txt
├── plugins/
└── build/
```

Typical commands:

```bash
cmake -S . -B build \
  -DSC_PATH=/path/to/supercollider

cmake --build build --config Release
```

The actual options depend on the supplied `CMakeLists.txt`. Do not copy generic build flags into a release guide until they have been tested with this project.

### 4. Test the result

- Copy the generated plugin extension into the user extensions directory.
- Recompile the class library.
- Boot `scsynth`.
- Run a silent or low-amplitude test.
- Check the server console for `exception in real time`, `UGen not installed`, symbol, or architecture errors.

### 5. Build targets

For macOS, clearly identify whether the product is:

```text
arm64
x86_64
Universal 2
```

For public distribution, publish separate platform assets or a verified Universal 2 build.

---

## Verification

After installation, verify the language and server separately.

### Language-class verification

A class can be located with:

```supercollider
HPExample.postln;
HPExample.filenameSymbol.postln;
```

Replace `HPExample` with the actual class name.

### UGen verification

Check whether a UGen class exists:

```supercollider
"HPExample".asClass.postln;
```

A language class existing does not prove that the server-side binary loaded correctly. Boot the server and run the project's smallest documented SynthDef.

### Server diagnostics

```supercollider
s.boot;
s.dumpOSC(1);
```

Use `s.dumpOSC(0)` to stop OSC logging.

For an audio test, always begin with conservative amplitude and a limiter.

---

## Using the classes and plugins

The exact API depends on the classes and UGens contained in the archives. Each public class or UGen should eventually have:

- a native `.schelp` help file;
- a one-block Quick Start;
- documented arguments and return values;
- a minimal musical example;
- a cleanup or stop example;
- version and dependency notes.

Example documentation pattern:

```supercollider
// Create or configure the object.
~object = HPExample.new;

// Run the smallest valid operation.
~object.start;

// Stop and release resources.
~object.stop;
```

Do not use this placeholder as an actual API unless the corresponding class implements these methods.

---

## Compatibility

The repository contains a package explicitly named for SuperCollider 3.10, while other archives do not currently expose their target version in the filename.

Maintain a tested compatibility table:

| Release | SuperCollider | macOS | Architecture | Windows | Linux |
|---|---|---|---|---|---|
| Legacy SC 3.10 package | 3.10 | To document | To document | To document | To document |
| Current package | To document | To document | To document | To document | To document |

A compatibility result should require at least:

1. class-library compilation;
2. server boot;
3. plugin loading;
4. one functional audio test;
5. normal stop and quit;
6. no unresolved architecture or symbol errors.

---

## Troubleshooting

### SuperCollider does not compile the class library

Likely causes:

- duplicate class name;
- obsolete syntax;
- missing dependency;
- class file in the wrong folder;
- accidental mixing of legacy and current versions.

Actions:

1. Copy the first error from the Post Window.
2. Locate the file and line number.
3. Remove duplicate or legacy copies.
4. Recompile.
5. Restart the IDE if the environment remains inconsistent.

### `Class not defined`

- Confirm that the `.sc` file is inside a configured extension path.
- Confirm filename and class name.
- Recompile the class library.
- Inspect `LanguageConfig.includePaths` and `excludePaths`.

### `UGen not installed`

The language wrapper was found, but the server plugin binary was not loaded.

Check:

- plugin location;
- CPU architecture;
- SuperCollider version;
- operating system;
- quarantine or code-signing problems;
- server restart after installation.

### Server crash or `exception in real time`

1. Stop using the plugin immediately.
2. Reproduce with the smallest SynthDef.
3. Reduce amplitude and channel count.
4. Record the SuperCollider and OS versions.
5. Record the complete server output.
6. Test the plugin build that matches the exact platform and architecture.

### Apple Silicon plugin does not load

Run:

```bash
file /path/to/Plugin.scx
```

If the output contains only `x86_64`, use an Intel SuperCollider process under Rosetta only as a temporary compatibility test, or rebuild the plugin for `arm64` / Universal 2.

### Help page is missing

Native help files must follow this layout:

```text
HelpSource/Classes/ClassName.schelp
```

Then rebuild the help index:

```supercollider
SCDoc.indexAllDocuments(true);
```

---

## Development and contributions

Contributions are welcome for:

- source-tree extraction and cleanup;
- SuperCollider 3.14 compatibility;
- Universal 2 plugin builds;
- native SCDoc help;
- CMake modernization;
- regression tests;
- verified examples;
- crash reports with minimal reproductions;
- macOS, Linux, and Windows compatibility reports.

A contribution should include:

```text
SuperCollider version
Operating system
CPU architecture
Compiler and version
CMake version
Plugin or class name
Steps to reproduce
Expected result
Actual result
Complete error output
```

Do not commit:

```text
.DS_Store
__MACOSX/
build/
*.o
*.dylib
*.so
*.dll
*.scx
large ZIP archives
private presets or sounds
```

Compiled products should be attached to GitHub Releases with SHA-256 checksums.

---

## Release process

Recommended first structured release:

```text
v0.9.0-beta.1
```

Recommended assets:

```text
HP-Class-and-Plugin-SC3.14-macOS-Universal2-v0.9.0-beta.1.zip
HP-Plugins-Source-v0.9.0-beta.1.zip
HP-Legacy-SC3.10-v0.9.0-beta.1.zip
SHA256SUMS.txt
```

Each Release should state:

- supported SuperCollider version;
- operating system;
- CPU architecture;
- signed/notarized status on macOS;
- required Quarks or plugins;
- known limitations;
- build instructions;
- migration notes;
- checksums.

Generate checksums on macOS:

```bash
shasum -a 256 *.zip > SHA256SUMS.txt
```

Verify:

```bash
shasum -a 256 -c SHA256SUMS.txt
```

---

## License

Add an explicit `LICENSE` file before encouraging redistribution or modification.

If the complete original collection can be licensed permissively, the MIT License is a simple option. However, verify every bundled third-party source, library, binary, and example before applying a repository-wide license.

A repository license cannot override the terms of SuperCollider, third-party UGens, included source code, or redistributed binaries.

---

## Acknowledgements

Built for [SuperCollider](https://supercollider.github.io/), an environment and programming language for real-time audio synthesis and algorithmic composition.

This project is maintained as part of the Provinescu software collection for experimental music and sound organization.
