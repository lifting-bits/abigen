# Introduction

This program is used to generate ABI libraries for [mcsema](https://github.com/trailofbits/mcsema).

# Usage

## Help
```
abigen generate --help
Using profiles from /usr/local/share/abigen/data/platforms
Generate an ABI library
Usage: abigen generate [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -p,--profile TEXT REQUIRED  Profile name; use the list_profiles command to list the available options
  -l,--language TEXT REQUIRED Language name; use the list_languages command to list the available options
  -x,--enable-gnu-extensions  Enable GNU extensions
  -i,--include-search-paths TEXT ...
                              Additional include folders
  -f,--header-folders TEXT ... REQUIRED
                              Header folders
  -b,--base-includes TEXT ... Includes that should always be present in the ABI header
  -o,--output TEXT REQUIRED   Output path, including the file name without the extension
```

## Example
```
abigen generate --profile 'Ubuntu 16.04.4 LTS' --language c11 --header-folders /path/to/library/include --output abi
```

# Generating new profiles

1. Install a new system from scratch, including the base headers
2. Get the base compiler settings: `clang -E -x c - -v < /dev/null` and `clang -E -x c++ - -v < /dev/null`
3. Create a new folder and profile file inside `abigen/data`

# Building instructions

## Clone the repository
`git clone --recurse-submodules https://github.com/trailofbits/abigen.git`

## If you didn't include '--recurse-submodules', initialize and update the submodules
`git submodule init`

`git submodule update --recursive`

## Install the depedencies
Up-to-date distributions like [Arch Linux](https://archlinux.org) will work out of the box by just installing `llvm` and `clang`.

If you are using a less updated system then you can use [cxx-common](https://github.com/trailofbits/cxx-common). Note that Ubuntu and Debian distributions are known to have shipped broken CMake files for LLVM and Clang in the past, so you are discouraged from using the repository packages.

**cxx-common for Ubuntu 14.04**
* [LLVM/Clang 3.5](https://s3.amazonaws.com/cxx-common/libraries-llvm35-ubuntu1404-amd64.tar.gz)
* [LLVM/Clang 3.6](https://s3.amazonaws.com/cxx-common/libraries-llvm36-ubuntu1404-amd64.tar.gz)
* [LLVM/Clang 3.7](https://s3.amazonaws.com/cxx-common/libraries-llvm37-ubuntu1404-amd64.tar.gz)
* [LLVM/Clang 3.8](https://s3.amazonaws.com/cxx-common/libraries-llvm38-ubuntu1404-amd64.tar.gz)
* [LLVM/Clang 3.9](https://s3.amazonaws.com/cxx-common/libraries-llvm39-ubuntu1404-amd64.tar.gz)
* [LLVM/Clang 4.0](https://s3.amazonaws.com/cxx-common/libraries-llvm40-ubuntu1404-amd64.tar.gz)
* [LLVM/Clang 5.0](https://s3.amazonaws.com/cxx-common/libraries-llvm50-ubuntu1404-amd64.tar.gz)

**cxx-common for Ubuntu 16.04**
* [LLVM/Clang 3.5](https://s3.amazonaws.com/cxx-common/libraries-llvm35-ubuntu1604-amd64.tar.gz)
* [LLVM/Clang 3.6](https://s3.amazonaws.com/cxx-common/libraries-llvm36-ubuntu1604-amd64.tar.gz)
* [LLVM/Clang 3.7](https://s3.amazonaws.com/cxx-common/libraries-llvm37-ubuntu1604-amd64.tar.gz)
* [LLVM/Clang 3.8](https://s3.amazonaws.com/cxx-common/libraries-llvm38-ubuntu1604-amd64.tar.gz)
* [LLVM/Clang 3.9](https://s3.amazonaws.com/cxx-common/libraries-llvm39-ubuntu1604-amd64.tar.gz)
* [LLVM/Clang 4.0](https://s3.amazonaws.com/cxx-common/libraries-llvm40-ubuntu1604-amd64.tar.gz)
* [LLVM/Clang 5.0](https://s3.amazonaws.com/cxx-common/libraries-llvm50-ubuntu1604-amd64.tar.gz)

Once you extract the package, set its path inside the `TRAILOFBITS_LIBRARIES` environment variable.

## Configure and build
`mkdir build && cd build`

`cmake /path/to/abigen/repository`

`cmake --build . --config Release`

## Installing/uninstalling

Note that you can run `abigen` directly (provided that you copied the `abigen/data` in the same directory) without installing anything on your system.

If you still want to install it, you can do so using the following commands:

`cmake --build . --config Release --target install`

`cmake --build . --config Release --target uninstall`