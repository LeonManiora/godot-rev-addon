#!/usr/bin/env python
import os
import sys

# Das sagt dem GitHub-Server, wo die Godot-Bindings liegen
env = SConscript("godot-cpp/SConstruct")

# Deine Quelldateien für den Compiler
src_files = [
    "src/register_types.cpp",
    "src/godot_rev_engine.cpp"
]

env.Append(CPPPATH=["src/"])

# Das baut die DLL am Ende in einen fertigen "bin" Ordner
target_lib = env.SharedLibrary(
    "bin/libgodot_rev",
    src_files
)

Default(target_lib)
