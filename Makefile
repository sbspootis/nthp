TARGET = run
CC = g++
SRCDIR = src/

VERSION = 1.0.3

# The linux buildsystem relies on a standard installation of SDL2 and SDL2_image
# (should it be used), with whatever default directories come with, as decided
# by the package manager. This buildsystem has no flexibility in regards to library paths.
# To use a custom installation/build of SDL2, the whole buildsystem has to be updated.


USE_SDLIMAGE = 1


ifeq ($(USE_SDLIMAGE), 1)

SDL_imageLibInclude = -lSDL2_image

else

SDL_imageLibInclude = 

endif


CFLAGS = -lSDL2 -lSDL2_mixer $(SDL_imageLibInclude) -D LINUX -D USE_SDLIMG=$(USE_SDLIMAGE)
DEBUG_CFLAGS = -g -lSDL2 -lSDL2_mixer $(SDL_imageLibInclude) -D LINUX -D DEBUG -DUSE_SDLIMG=$(USE_SDLIMAGE)
libTargets = global_defs.o core.o position.o palette.o rawsurface.o softwaretexture.o e_entity.o e_collision.o st_compress.o s_compiler.o s_script.o gtexture.o s_linker.o s_runtime.o st_mono.o
debug_libTargets = global_defs_d.o core_d.o position_d.o palette_d.o rawsurface_d.o softwaretexture_d.o e_entity_d.o e_collision_d.o st_compress_d.o s_compiler_d.o s_script_d.o gtexture_d.o s_linker_d.o s_runtime_d.o st_mono_d.o


lib_srcSymbols = $(SRCDIR)global_defs.cpp $(SRCDIR)core.cpp $(SRCDIR)position.cpp $(SRCDIR)palette.cpp $(SRCDIR)rawsurface.cpp $(SRCDIR)softwaretexture.cpp $(SRCDIR)e_entity.cpp $(SRCDIR)e_collision.cpp $(SRCDIR)st_compress.cpp $(SRCDIR)s_compiler.cpp $(SRCDIR)s_script.cpp $(SRCDIR)gtexture.cpp $(SRCDIR)s_linker.cpp $(SRCDIR)s_runtime.cpp $(SRCDIR)st_mono.cpp


PM_CFLAGS = -lSDL2 -lSDL2_mixer $(SDL_imageLibInclude) -D LINUX -D DEBUG -D USE_SDLIMG=$(USE_SDLIMAGE)

all: release debug pm


release: $(libTargets) main.o
	$(CC) $(libTargets) main.o  $(CFLAGS) -o $(TARGET)


debug: $(debug_libTargets) main_d.o 
	$(CC) $(debug_libTargets) main_d.o $(DEBUG_CFLAGS) -o $(TARGET)_debug



# RELEASE DEPENDENCIES GO HERE VVV

main.o: $(SRCDIR)main.cpp
	$(CC) $(CFLAGS) -c $(SRCDIR)main.cpp -o main.o

global_defs.o: $(SRCDIR)global_defs.cpp $(SRCDIR)global.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)global_defs.cpp -o global_defs.o

core.o: $(SRCDIR)core.cpp $(SRCDIR)core.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)core.cpp -o core.o

position.o: $(SRCDIR)position.cpp $(SRCDIR)position.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)position.cpp -o position.o


palette.o: $(SRCDIR)palette.cpp $(SRCDIR)palette.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)palette.cpp -o palette.o


rawsurface.o: $(SRCDIR)rawsurface.cpp $(SRCDIR)rawsurface.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)rawsurface.cpp -o rawsurface.o

softwaretexture.o: $(SRCDIR)softwaretexture.cpp $(SRCDIR)softwaretexture.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)softwaretexture.cpp -o softwaretexture.o

e_entity.o: $(SRCDIR)e_entity.cpp $(SRCDIR)e_entity.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)e_entity.cpp -o e_entity.o


e_collision.o: $(SRCDIR)e_collision.hpp $(SRCDIR)e_collision.cpp
	$(CC) $(CFLAGS) -c $(SRCDIR)e_collision.cpp -o e_collision.o

st_compress.o: $(SRCDIR)st_compress.cpp $(SRCDIR)st_compress.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)st_compress.cpp -o st_compress.o


s_compiler.o: $(SRCDIR)s_compiler.hpp $(SRCDIR)s_compiler.cpp
	$(CC) $(CFLAGS) -c $(SRCDIR)s_compiler.cpp -o s_compiler.o
	

s_script.o: $(SRCDIR)s_script.cpp $(SRCDIR)s_script.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)s_script.cpp -o s_script.o


gtexture.o: $(SRCDIR)gtexture.hpp $(SRCDIR)gtexture.cpp
	$(CC) $(CFLAGS) -c $(SRCDIR)gtexture.cpp -o gtexture.o


t_font.o: $(SRCDIR)t_font.hpp $(SRCDIR)t_font.cpp
	$(CC) $(CFLAGS) -c $(SRCDIR)t_font.cpp -o t_font.o

s_linker.o: $(SRCDIR)s_linker.hpp $(SRCDIR)s_linker.cpp
	$(CC) $(CFLAGS) -c $(SRCDIR)s_linker.cpp -o s_linker.o

s_runtime.o: $(SRCDIR)s_runtime.hpp $(SRCDIR)s_runtime.cpp
	$(CC) $(CFLAGS) -c $(SRCDIR)s_runtime.cpp -o s_runtime.o


st_mono.o: $(SRCDIR)st_mono.hpp $(SRCDIR)st_mono.cpp
	$(CC) $(CFLAGS) -c $(SRCDIR)st_mono.cpp -o st_mono.o

# DEBUG DEPENDENCIES GO HERE VVV


main_d.o: $(SRCDIR)main.cpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)main.cpp -o main_d.o

global_defs_d.o: $(SRCDIR)global_defs.cpp $(SRCDIR)global.hpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)global_defs.cpp -o global_defs_d.o

core_d.o: $(SRCDIR)core.cpp $(SRCDIR)core.hpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)core.cpp -o core_d.o

position_d.o: $(SRCDIR)position.cpp $(SRCDIR)position.hpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)position.cpp -o position_d.o


palette_d.o: $(SRCDIR)palette.cpp $(SRCDIR)palette.hpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)palette.cpp -o palette_d.o

rawsurface_d.o: $(SRCDIR)rawsurface.cpp $(SRCDIR)rawsurface.hpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)rawsurface.cpp -o rawsurface_d.o

softwaretexture_d.o: $(SRCDIR)softwaretexture.cpp $(SRCDIR)softwaretexture.hpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)softwaretexture.cpp -o softwaretexture_d.o

e_entity_d.o: $(SRCDIR)e_entity.cpp $(SRCDIR)e_entity.hpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)e_entity.cpp -o e_entity_d.o


e_collision_d.o: $(SRCDIR)e_collision.hpp $(SRCDIR)e_collision.cpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)e_collision.cpp -o e_collision_d.o

st_compress_d.o: $(SRCDIR)st_compress.cpp $(SRCDIR)st_compress.hpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)st_compress.cpp -o st_compress_d.o


s_compiler_d.o: $(SRCDIR)s_compiler.hpp $(SRCDIR)s_compiler.cpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)s_compiler.cpp -o s_compiler_d.o
	

s_script_d.o: $(SRCDIR)s_script.cpp $(SRCDIR)s_script.hpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)s_script.cpp -o s_script_d.o

gtexture_d.o: $(SRCDIR)gtexture.hpp $(SRCDIR)gtexture.cpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)gtexture.cpp -o gtexture_d.o


t_font_d.o: $(SRCDIR)t_font.hpp $(SRCDIR)t_font.cpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)t_font.cpp -o t_font_d.o

s_linker_d.o: $(SRCDIR)s_linker.hpp $(SRCDIR)s_linker.cpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)s_linker.cpp -o s_linker_d.o

s_runtime_d.o: $(SRCDIR)s_runtime.hpp $(SRCDIR)s_runtime.cpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)s_runtime.cpp -o s_runtime_d.o

st_mono_d.o: $(SRCDIR)st_mono.hpp $(SRCDIR)st_mono.cpp
	$(CC) $(DEBUG_CFLAGS) -c $(SRCDIR)st_mono.cpp -o st_mono_d.o




# PM DEPENDENCIES GO HERE ||
# 			  VV






pm: $(SRCDIR)pm_globals.cpp $(SRCDIR)pm_core.cpp
	$(CC) $(SRCDIR)pm_globals.cpp $(SRCDIR)pm_core.cpp $(lib_srcSymbols) $(CFLAGS) -D PM  -o pm









clean:
	@rm -f *.o *~ new_core.*

