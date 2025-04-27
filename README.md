# MyJVM

MyJVM is a lightweight, standalone CLI tool for compiling and running simple Java, Scala 3, and Kotlin projects without any external setup.
It bundles its own runtimes and compilers internally, making it easy to prototype small JVM programs from anywhere.

## Features

 - 📦 Fully self-contained: No need to install Java, Scala, or Kotlin separately
 - ⚡ Fast compilation: Local lightweight runtimes — no downloading dependencies at runtime
 - 📂 Supports .java, .scala, and .kt files
 - 🏗️ Automatic runtime extraction if not already present
 - 🔧 Simple command-line usage (no SBT, Gradle, or Maven needed)
 - 🎯 Minimal disk space usage (around 136MB for all languages combined)
 - 🛠️ Automatic JAR packaging for easier execution
 - 🔄 Multiple file support (including cross-referencing classes in small projects)

## Usage

### Building
```bash
g++ -std=c++20 -O3 \
  -I/opt/homebrew/include \
  -I/opt/homebrew/Cellar/libarchive/3.7.9/include \
  src/main.cpp \
  src/os.cpp \
  src/utils.cpp \
  -L/opt/homebrew/lib \
  -larchive \
  -lzstd \
  -o build/myjvm
# (adjust include and library paths if needed)
```

### Running

To run the tool from its build directory, use:
```bash
./build/myjvm test 
# or
./build/myjvm test example.MainProgram
````
If a .languages/ directory doesn’t exist, the tool automatically extracts the JVM runtimes.
If no file/class is specified, it will compile all files, then attempt to run all class names from the given directory.

## Supported Languages

| Language | Compilation Method                   | Notes                                    | 
| -------- | ------------------------------------ | ---------------------------------------- | 
| Java     | Compiled + JAR packed                | Standard class/method entrypoints        | 
| Scala3   | Compiled with bundled Scala compiler	| Requires proper object Main { def main } |
| Kotlin   | Compiled and run as fat JAR          | Single project structure per run         | 

## Current Limitations

 - Primarily designed for small projects (scripts, demos, quick utilities)
 - No external dependency resolution yet (like Maven Central, Ivy, etc.)
 - Spark / heavy frameworks not supported (requires broader environment management)
 - Kotlin must be treated as a single-entry project for now (running via -jar)

## Future Ideas

 - Dependency resolution (like SBT/Gradle minimal subset)
 - Smarter automatic class detection (for Kotlin multi-entry)
 - Stress test and optimize runtime handling

## Why MyJVM?

Setting up and managing full toolchains for different JVM languages can be overkill for small experiments.

MyJVM lets you focus purely on your code — no project files, no build scripts, no dependency management nightmares.

It’s like a personal JVM sandbox, made simple.

## License

 - This project is experimental and provided as-is.
 - You must ensure that any language runtimes or libraries used comply with their original licenses.
 - Any licenses included from JVM runtimes and libraries are available in ./licenses/

