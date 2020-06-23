# stockfish-nnue
Stockfish NNUE (efficiently updateable neural network)

forked from:
https://github.com/nodchip/Stockfish/releases/tag/stockfish-nnue-2020-06-09

Clang-tidy recommendations/fixes applied:

-Use auto when initializing with a cast to avoid duplicating the type name [modernize-use-auto]
-auto can be declared as 'auto *' [readability-qualified-auto]
-Use emplace_back instead of push_back [modernize-use-emplace]
-Parameter may be const
-Functional-style cast used instead of a C++ cast
-Local variable may be const
-Member function may be 'const'
-Member function may be 'static'
-C-style cast used instead of a C++ cast
-Functional-style cast used instead of a C++ cast
-Include guard not found at the beginning of a header file
-Possibly unused #include directive
-Redundant parentheses
-Zero constant can be replaced with nullptr
-Use range-based for loop instead [modernize-loop-convert]
-The 'empty' method should be used to check for emptiness instead of 'size' [readability-container-size-empty]
-Redundant declaration [readability-redundant-declaration]

Modernized from c++14 to c++17
Translated to English

Windows 64-bit pext & popcnt (Intel-compiled) executables included:

Please see
README.txt
README_TOO.md
for more info

Try nnue-gui:
https://github.com/FireFather/nnue-gui

It's a basic GUI to keep track of settings, paths, UCI options, command line parameters, etc.
It will also launch the various binaries needed for all 3 phases: gen training data, gen validation data, and learning (training).