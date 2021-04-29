cmake_minimum_required(VERSION 3.18.1)
set(CMAKE_VERBOSE_MAKEFILE on)

project(png)

include("../../../../../hltypes/android-studio/generic/CMakeLists.txt")

# defining manually so source files don't need to be changed
file(
	GLOB CppSrc
	"../../png.c"
	"../../pngerror.c"
	"../../pngget.c"
	"../../pngmem.c"
	"../../pngpread.c"
	"../../pngread.c"
	"../../pngrio.c"
	"../../pngrtran.c"
	"../../pngrutil.c"
	"../../pngset.c"
	"../../pngtrans.c"
	"../../pngwio.c"
	"../../pngwrite.c"
	"../../pngwtran.c"
	"../../pngwutil.c"
	"../../arm/arm_init.c"
	"../../arm/filter_neon_intrinsics.c"
	"../../arm/palette_neon_intrinsics.c"
	"../../arm/filter_neon.S"
)

add_library(png STATIC ${CppSrc})

string(REPLACE " -mfpu=vfpv3-d16" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE " -mfpu=vfpv3-d16" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE " -mfpu=vfpv3-d16" "" CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}")

include_directories(AFTER "../../../hltypes/lib/zlib")

target_link_libraries(
	png
)
