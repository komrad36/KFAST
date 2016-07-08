EXECUTABLE_NAME=KFAST
CC=g++
INC=
CFLAGS=-Wall -Werror -Wextra -Wshadow -pedantic -Ofast -std=gnu++17 -fomit-frame-pointer -mavx2 -msse4 -march=native -mfma -flto
CCLIBS=
LDFLAGS=-Wall -Werror -Wextra -Wshadow -pedantic -Ofast -std=gnu++17 -fomit-frame-pointer -mavx2 -msse4 -march=native -mfma -flto
LIBS=-lopencv_core -lopencv_features2d -lopencv_highgui -lopencv_imgcodecs
CPPSOURCES=$(wildcard *.cpp)

OBJECTS=$(CPPSOURCES:.cpp=.o)

.PHONY : all
all:
	$(MAKE) standard PROFILE=-fprofile-use

.PHONY : standard
standard: $(CPPSOURCES) $(EXECUTABLE_NAME)

$(EXECUTABLE_NAME) : $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(PROFILE) -o $@ $(LIBS)

%.o:%.cpp
	$(CC) -c $(INC) $(CFLAGS) $(PROFILE) $< -o $@

.PHONY : profile
profile: 
	$(MAKE) standard PROFILE=-fprofile-generate

.PHONY : clean
clean:
	rm -rf *.o $(EXECUTABLE_NAME)
