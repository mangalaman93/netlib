NetLib
======
Basic Network Library based on Boost framework.

Dependency
==========
* g++
* Boost framework (will be automatically installed by `make` utility)
* [Crypto++](http://www.cryptopp.com/)

Configuration possibilities
===========================
* TCP timeout before keep alive probes are sent [tcp_connection.cpp]
* Wait time while waiting for io thread to set up the connection [base_network.h]

How to run (Linux)
=================
Simply run `make` to compile all the examples.

Compile boost on Windows
========================
```
bootstrap.bat
b2.exe --with-system --with-thread --with-date_time --with-regex --prefix=<path>
```

Visual Studio Project
=====================
* Open the visual studio project from project/netlib.vcxproj
* Compile boost library (preferably version 1.55) and crpto++ libraries
* Compile the `ed25519` project present in `lib/ed25519` folder and copy the `ed25519.lib` file in `lib` folder
* copy `cryptlib.lib` file in `lib` folder
* copy `lib/ed25519` folder and `crypto++` library folder to `include/` folder (for .h include files)
* Open project properties and add `include/` folder, `<path to boost library>` to `Configuration Properties -> C/C++ -> Additional Include Directories`
* Add `<path to compiled boost libs>` and `lib` folder to `Configuration Properties -> Linker -> Additional Library Directories`
* Choose files from examples and add to project to run test one by one
* Note that the cryptopp library should be extracted to folder named `cryptopp`
