CPP = g++
RM = rm
MKDIR = mkdir
CPPFLAGS = -I/mingw/SFML-1.6/include -I/mingw/include/ -I/mingw/boost_1_49_0 -std=gnu++0x
LDFLAGS = -static-libstdc++ -static-libgcc -lsfml-audio-s -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -lmingw32 -lopengl32 -lglu32 -L/mingw/SFML-1.6/lib -L/mingw/lib -L/mingw/boost_1_49_0/libs -O0 -Wall -fmessage-length=0 -mwindows

all: build/main.exe

build/main.exe: main.cpp Makefile
	$(CPP) -o build/main.exe main.cpp $(LDFLAGS) $(CPPFLAGS)

clean:
	test -d $(OBJDIR) || $(RM) -rf $(OBJDIR)
	$(RM) build/main.exe