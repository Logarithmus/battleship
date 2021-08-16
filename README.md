[![Actions Status](https://github.com/Logarithmus/battleship/workflows/Ubuntu/badge.svg)](https://github.com/Logarithmus/battleship/actions)
[![Actions Status](https://github.com/Logarithmus/battleship/workflows/Windows/badge.svg)](https://github.com/Logarithmus/battleship/actions)
[![Actions Status](https://github.com/Logarithmus/battleship/workflows/MacOS/badge.svg)](https://github.com/Logarithmus/battleship/actions)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Logarithmus/battleship)](https://github.com/Logarithmus/battleship/releases)

# Battleship game
Yet another implementation of Battleship game in C++ with GUI in SFML & server to play with someone else.

## Roadmap
- [x] Common data structures & algorithms (`Grid`, `PlayerField`, etc.)
- [ ] Server (WIP)
- [ ] Client library (WIP)
- [ ] SFML GUI (WIP)

### Prerequisites

You will need:

* **CMake v3.15+** - found at [https://cmake.org/](https://cmake.org/)
* **C++ Compiler** - needs to support the **C++20** standard, i.e. the latest version of *MSVC*,
*GCC*, *Clang* would suffice.

> ***Note:*** *You also need to be able to provide ***CMake*** a supported
[generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html).*

## Building the project

To build the project, you need to run the following command:

```bash
mkdir build/ && cd build/ && cmake .. && make
```

### Code style

Code style is enforced by `clang-format`. So make sure to run it on your code before contributing.

## Versioning

This project makes use of [SemVer](http://semver.org/) for versioning. A list of
existing versions can be found in the
[project's releases](https://github.com/Logarithmus/battleship/releases).

## License

This project is licensed under AGPLv3. See [LICENSE](LICENSE) file for details.
