EXECUTABLE_NAME=KFAST
CC=g++
INC=
CFLAGS=-Wall -Wextra -Werror -Wshadow -pedantic -Ofast -std=gnu++17 -fomit-frame-pointer -mavx2 -march=native -mfma -flto -funroll-all-loops -fpeel-loops -ftracer -ftree-vectorize
LIBS=-lopencv_core -lopencv_features2d -lopencv_highgui -lopencv_imgcodecs -lpthread
CPPSOURCES=$(wildcard *.cpp)

OBJECTS=$(CPPSOURCES:.cpp=.o)

.PHONY : all
all: $(CPPSOURCES) $(EXECUTABLE_NAME)

$(EXECUTABLE_NAME) : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(PROFILE) -o $@ $(LIBS)

%.o:%.cpp
	$(CC) -c $(INC) $(CFLAGS) $(PROFILE) $< -o $@

.PHONY : clean
clean:
	rm -rf *.o $(EXECUTABLE_NAME)
