[![Actions Status](https://github.com/Logarithmus/battleship/workflows/Ubuntu/badge.svg)](https://github.com/Logarithmus/battleship/actions)
[![Actions Status](https://github.com/Logarithmus/battleship/workflows/Windows/badge.svg)](https://github.com/Logarithmus/battleship/actions)
[![Actions Status](https://github.com/Logarithmus/battleship/workflows/MacOS/badge.svg)](https://github.com/Logarithmus/battleship/actions)
[![codecov](https://codecov.io/gh/Logarithmus/battleship/branch/master/graph/badge.svg)](https://codecov.io/gh/Logarithmus/battleship)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Logarithmus/battleship)](https://github.com/Logarithmus/battleship/releases)

# Battleship game
Yet another implementation of Battleship game in C++ with GUI in SFML & server to play with someone else.

## Roadmap
[] Common data structures & algorithms (`Grid`, `PlayerGrid`, etc.)
[] Server
[] SFML GUI

### Prerequisites

You will need:

* **CMake v3.15+** - found at [https://cmake.org/](https://cmake.org/)
* **C++ Compiler** - needs to support the **C++20** standard, i.e. *MSVC*,
*GCC*, *Clang*

> ***Note:*** *You also need to be able to provide ***CMake*** a supported
[generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html).*

## Building the project

To build the project, you need to run the following command:

```bash
mkdir build/ && cd build/ && cmake .. && make
```

## Generating the documentation

In order to generate documentation for the project, you need to configure the build
to use Doxygen. This is easily done, by modifying the workflow shown above as follows:

```bash
mkdir build/ && cd build/
cmake .. -D<project_name>_ENABLE_DOXYGEN=1 -DCMAKE_INSTALL_PREFIX=/absolute/path/to/custom/install/directory
cmake --build . --target doxygen-docs
```

> ***Note:*** *This will generate a `docs/` directory in the **project's root directory**.*

## Running the tests

This project uses [Google Test](https://github.com/google/googletest/)
for unit testing. Unit testing can be disabled in the options, by setting the
`ENABLE_UNIT_TESTING` (from
[cmake/StandardSettings.cmake](cmake/StandardSettings.cmake)) to be false. To run
the tests, simply use CTest, from the build directory, passing the desire
configuration for which to run tests for. An example of this procedure is:

```bash
cd build          # if not in the build directory already
ctest -C Release  # or `ctest -C Debug` or any other configuration you wish to test

# you can also run tests with the `-VV` flag for a more verbose output (i.e.
#GoogleTest output as well)
```
### Coding style tests

Code style is enforced by `clang-format`. So make sure to run it on your code before contributing.

## Versioning

This project makes use of [SemVer](http://semver.org/) for versioning. A list of
existing versions can be found in the
[project's releases](https://github.com/filipdutescu/modern-cpp-template/releases).

## License

This project is licensed under AGPLv3. See [LICENSE](LICENSE) file for details.
