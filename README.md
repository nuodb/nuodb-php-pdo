# NuoDB PHP PDO Native Driver #

## Installing Necessary Dependencies ##

In order to build the driver you must have PHP plus developer tools installed.
This may be done from source or using package managers.

n.b. PHP version 5.4.3 is required. It is unlikely that PHP distributions from
     system package managers such as YUM or APT will have a recent enough version
     For this reason we strongly recommend installing PHP from source.

There is one prerequisite for all Linux systems:

```bash
  sudo apt-get install libxml2 libxml2-dev
```

### Installing from Source ###

To install PHP on a Linux system use the following commands:

```bash
  wget http://www.php.net/distributions/php-5.4.3.tar.gz
  tar xzvf php-5.4.3.tar.gz
  cd php-5.4.3
  ./configure --enable-shared --with-zlib --enable-cli --enable-pdo
  make
  sudo make install
```

## PROVIDING NECESSARY LINK LIBRARIES AND INCLUDE DIRECTORIES ##

The API must be linked against the existing Remote libraries, and requires
the includes shipped with the product. Export the following environment
variables:

```bash
  export NUODB_LIB_DIR=/opt/nuodb/lib64
  export NUODB_INCLUDE_DIR=/opt/nuodb/include
  export PHP_INCLUDE_DIR=/usr/local/include/php
```

Windows is slightly different:

```bash
  set NUODB_LIB_DIR=C:\Dev\Software\NuoDB\lib
  set NUODB_INCLUDE_DIR=C:\Dev\Software\NuoDB\include
  set PHP_INCLUDE_DIR=/usr/local/include/php
```

## GENERATING MAKEFILES ##

Currently we support building the API on both Windows and Unix, supporting GNU
makefiles, and both Xcode and Visual Studio 2010 projects. On Mac, your option
is to use either Xcode or GNU makefiles.

Run the following command to generate GNU makefiles: 

```bash
  cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=RelWithDebInfo .
```

Run the following command to generate Xcode project files:

```bash
  cmake -G Xcode
```

Run the following command to generate Visual Studio 2010 project
files:

```bash
  cmake -G "Visual Studio 10" -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

## BUILDING ##

The cmake build system is chained to master on the nimbusdb repository
via git submodules. The dependent artifacts are built first followed
by the C++ API.

Run the following command to perform a GNU Make based build:

```bash
  make
```

Run the following command to perform an Xcode build:

```bash
  xcodebuild
````

Run the following command to perform a Visual Studio build:

```bash
  build-vs.bat
````

To build on Solaris, install all GNU tools, including GNU Make; do NOT
use the `make` command as this is from the Sun Tool chain. To build on
Solaris run the following make command instead:

```bash
  gmake
```

## CLEANING ##

Run the following command to clean the build area:

```bash
  make clean
```

## REGENERATING MAKEFILES ##

Though generally unnecessary, in some cases after modifying CMake files
you may need to regenerate all generated files. Run the following command
then generate makefiles according to above to force regeneration:

```bash
  make clean
  rm CMakeCache.txt
```

## RUNNING TESTS ##

Prerequisites for running the unit tests include having a running chorus; a
minimal chorus can be started using these commands:

```bash
java -jar nuoagent.jar --broker --domain test --password test &
./nuodb --chorus flights --password planes --dba-user cloud --dba-password user &
```

Run the following commands to run the tests:

```bash
  !!!!!! TODO DOC HERE !!!!!!
```

## CREATING A DISTRIBUTION PACKAGE ##

After running a build, run the following command to build a binary distribution:

```bash
  cpack -G TGZ
```

Similarly, on Windows run the following command:

```bash
  cpack -G ZIP
```
