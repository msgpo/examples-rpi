
# test for presence of SDL

ifeq ($(origin SDL_CFLAGS) $(origin SDL_LDLIBS), undefined undefined)
  SDL_CONFIG = $(CROSS_COMPILE)sdl2-config
  ifeq ($(shell which $(SDL_CONFIG) 2>/dev/null),)
    SDL_CONFIG = $(CROSS_COMPILE)sdl-config
    ifeq ($(shell which $(SDL_CONFIG) 2>/dev/null),)
      $(error No SDL development libraries found!)
    else
      $(warning Using SDL 1.2 libraries)
    endif
  endif
  SDL_CFLAGS  += $(shell $(SDL_CONFIG) --cflags)
  SDL_LDLIBS += $(shell $(SDL_CONFIG) --libs)
endif
CFLAGS += $(SDL_CFLAGS)
LDLIBS += $(SDL_LDLIBS)

all: keyboard video rumble

keyboard: keyboard.c
	gcc keyboard.c -I /usr/local/include/SDL2 -o keyboard $(CFLAGS) $(LDLIBS)

video:
	gcc video.c -I /usr/local/include/SDL2 -o video $(CFLAGS) $(LDLIBS)

rumble: rumble.c
	gcc rumble.c -I /usr/local/include/SDL2 -o rumble $(CFLAGS) $(LDLIBS)
