SOURCE := src/
INCLUDE := includes/
OBJ := objects/
TARGET := Webserver.bin
DELETE := rm
CC := gcc
CFLAGSL := $(CC) -g -Wall -Werror -fPIC
CFLAGSO := $(CC) -g -c -Wall -Werror -fPIC


default: $(TARGET)
all: default

$(TARGET): $(OBJ)Main.o
	$(CFLAGSL) $(OBJ)Main.o -o $(TARGET)

$(OBJ)Main.o: $(SOURCE)Main.c
	$(CFLAGSO) $(SOURCE)Main.c -I $(INCLUDE) -o $(OBJ)Main.o

clean:
	-$(DELETE) $(OBJ)Main.o $(TARGET)