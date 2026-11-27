# Classes and Plugins for SuperCollider

> **Provinescu Software Production**
> Experimental tools for composition, sound organization, synthesis, and live performance.

**SuperCollider 3.14 · macOS · Apple Silicon and Intel · Generative Music**

This repository brings together SuperCollider classes, tools written in SuperCollider, and native audio plugins developed as part of my practice as a **drummer, composer, and software developer**.

## Overview

These tools are designed to organize sound, build generative processes, transform audio, and create systems that can function as partners in the compositional process.

## Repository contents

- Software classes for SuperCollider
- Extensions required by the classes
- Native audio plugins
- Wek editions integrating Wekinator
- TR editions integrating HPtransformer
- Wek+TR editions combining Wekinator and HPtransformer
- Launchers and supporting tools

## Available editions

| Edition | Typical name | Wekinator | HPtransformer | Main function |
|---|---|:---:|:---:|---|
| **Standard** | `Agents` | No | No | Standard functionality |
| **Wek** | `WekAgents` | Yes | No | Parameter control via Wekinator |
| **TR** | `AgentsTR` | No | Yes | Learning and generation with HPtransformer |
| **Wek+TR** | `WekAgentsTR` | Yes | Yes | Combines Wekinator and HPtransformer |

The `Wek` prefix identifies an edition that communicates with Wekinator. The `TR` suffix identifies an edition that integrates HPtransformer.

## Requirements

- SuperCollider 3.14
- Contents of the `Extensions` folder supplied with the repository
- A MIDI source enabled before launch
- [VSTPlugin](https://github.com/Spacechild1/vstplugin) for classes using VST plugin hosting
- [Wekinator](https://github.com/fiebrink1/wekinator) for Wek and Wek+TR editions

## Installation

1. Quit SuperCollider.
2. Copy the required classes and extensions into your SuperCollider user `Extensions` folder.
3. Restart SuperCollider and recompile the class library.
4. Check the Post window for compilation errors.
5. Enable a MIDI source before launching the software.

## Quick start

Evaluate the constructor of the class you want to use:

```supercollider
Agents.new;
```

The first time it is launched, the software creates a working folder named after the program in your `Documents` folder. It contains presets, data, sound lists, and configuration files.

## Custom sounds

Some programs load sounds into memory and may be sensitive to the number or size of the selected files.

To use your own sounds:

1. Create a new working folder.
2. Allow the software to initialize the required files.
3. Locate the `sound` or `list sound` file.
4. Make a backup copy.
5. Edit the file while preserving its syntax exactly.
6. Restart the software or reload its sound configuration.

On macOS, Spotlight's `mdfind` command can help locate files that have been moved. On Linux and Windows, all saved file paths must be valid and accessible.

## Performance and diagnostic messages

TR and Wek+TR editions require more real-time processing than Standard and Wek editions.

Depending on audio-server latency and processor load, messages such as `late`, `node not found`, or `buffer` may occasionally appear in the SuperCollider Post window.

If the software and audio continue to operate correctly, these messages are generally non-critical.

## Documentation

See:

- [HP Software Guides](https://github.com/provinescu/HP-Software-Guides)
- [Wekinator Models for HP Software](https://github.com/provinescu/Wekinator-Models-for-HP-Software)

## Project philosophy

An artistic and software project dedicated to experimental music, generative systems, and sound organization. The project is developed by a drummer, composer, and software developer.

Explore the parameters, listen to the results, save useful configurations, and adapt the systems to your own musical practice.

- [GitHub profile](https://github.com/provinescu)
- [Music on Bandcamp](https://provinescu.bandcamp.com/)

## License

See the repository's `LICENSE` file for details.
