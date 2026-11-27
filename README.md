# Classes and Plugins for SuperCollider

> **Provinescu Software Production**  
> Independent classes, utilities, UGens, and audio plugins for direct use in SuperCollider.

**SuperCollider 3.14 · macOS · Apple silicon and Intel**

This repository contains independent tools for sound synthesis, signal processing, algorithmic composition, and musical experimentation.

These components are not part of the HP standalone applications or the HP Software Classes collection. Each class or plugin can be used independently according to its own requirements.

## Contents

- SuperCollider classes and language-side utilities
- UGens and native audio plugins
- Synthesis and signal-processing tools
- Algorithmic-composition tools
- Examples and supporting files

## Installation

1. Quit SuperCollider.
2. Copy the required files into your SuperCollider user `Extensions` folder.
3. Restart SuperCollider and recompile the class library.
4. Check the Post window for errors.

Typical locations:

```text
macOS:   ~/Library/Application Support/SuperCollider/Extensions/
Linux:   ~/.local/share/SuperCollider/Extensions/
Windows: %LOCALAPPDATA%/SuperCollider/Extensions/
```

Native plugins must match your operating system, processor architecture, and SuperCollider version.

## Usage

Consult the example, help file, or source comments supplied with each component. Constructors, arguments, dependencies, and server requirements vary between classes and plugins.

Example:

```supercollider
MyClass.new;
```

Some components may require additional dependencies such as [VSTPlugin](https://github.com/Spacechild1/vstplugin).

## Troubleshooting

If a class or plugin is unavailable:

- confirm that its files are inside a scanned `Extensions` folder;
- recompile the class library;
- check the Post window for compilation or plugin-loading errors;
- verify all required dependencies;
- confirm compatibility with your operating system and processor architecture.

## About Provinescu

**Provinescu** is an artistic and software project dedicated to experimental music, generative systems, sound synthesis, and sound organization.

- [GitHub profile](https://github.com/provinescu)
- [Music on Bandcamp](https://provinescu.bandcamp.com/)

## License

See the repository's `LICENSE` file. Individual components may include their own licensing terms.
