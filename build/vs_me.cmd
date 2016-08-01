REM Use Cmake to generate Visual Studio (VS) build environment
SET BUILD_DIR=build_vs
if exist %BUILD_DIR% (
	rmdir /s/q %BUILD_DIR%
)
mkdir %BUILD_DIR%
cd %BUILD_DIR%
REM use Cmake to generate default build environment (on windows it is Visual Studio)
cmake ..
cd ..