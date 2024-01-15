# Evey
Evey is an open source project for leading edge video codec. Evey is intended not to be a reference implementation of a particular video coding standard but to be a continuously evolving framework which can be used as a starting point of next generation of video coding standards development.

Major strong features of Evey includes:
1) Royalty-free coding tools invented or published in more than two decades ago.
2) Flexible block splitting algorithm to divide a large picture into small coding blocks.
3) High speed implementation of algorithms using multi-core and SIMD instruction sets for reducing turnaround times of time consuming experiments.
4) Well written native C-language source code for excellent readability and easy conversion to other languages.
5) Providing a special tool for renaming the project name and source code to create and start a new project.

To maintain the reusability of the basic architecture of Evey and its royalty free nature, it is highly recommended to fork Evey project whenever an experiment targeting new video coding standard is initiated.

For the purpose of portability, Evey project includes a special tool for cloning the source code with different project name. By using the tool, Evey project can be easily converted to new project with a given name.

## How to build

### Linux (64-bit)
- Build Requirements
  - CMake 3.5 or later (download from [https://cmake.org/](https://cmake.org/))
  - GCC 5.4.0 or later
  
- Build Instructions
  ```
  $mkdir build
  $cd build
  $cmake ..
  $make
  ```
  - Output Location
    - Executable applications can be found under build/bin/.

### Windows (64-bit)
- Build Requirements
  - CMake 3.5 or later (download from [https://cmake.org/](https://cmake.org/))
  - MinGW-64 or Microsoft Visual Studio

- Build Instructions
  - MinGW-64
    ```
    $mkdir build
    $cd build
    $cmake .. -G "MinGW Makefiles"
    $make
    ```
  - Microsoft Visual Studio 
    ```
    $mkdir build
    $cd build
    $cmake .. -G "Visual Studio 15 2017 Win64"
    $make
    ```
    You can change '-G' option with proper version of Visual Studio.

## How to use

### Examples
- Encoder example
```
$eveya_encoder --config {config.cfg} -i {input.yuv} -w {width} -h {height} -z {fps} -q {qp} -f {frames} -s -o {output.bin}
```
- Decoder example
```
$eveya_decoder -s -i {input}.bin -o {ouptut}.yuv
```

## How to change the project name
- Requirements
  - Python 2.7 or higher
- Instructions
  - goto 'tools' folder
  - Execute script
    ```
    $python refactor_project.py -i ../ -o output -n newcodec --readme-content "This is a new project named newcodec"
    ```
  - In 'output' directory, you can find the new source code files cloned from Evey project. The script changes all file names, function names, definitions, build target names and so on. 


## License
See [COPYING](COPYING) file for details.


