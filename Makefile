# the make file

EXECUTABLE=evaluator# nombre del ejecutable a generar

## CONFIG:
#SRC=src # carpeta con fichero fuente
# comando
LRT=lrt
# comando
LPTHREAD=lpthread
# especificar con que version compilar
VERSION=std=c++11
#compilador
COMPILER=g++
#carpeta fuernte con fichero
SOURCE=src/EvalMain.cpp


# Folder donde se guardan los binarios
BIN=bin

ARGS=-$(COMPILER) -o $(EXECUTABLE) $(SOURCE) -$(LRT) -$(LPTHREAD) -$(VERSION)

PWD = $(shell pwd)/$(BIN)

DIRECTORY = export PATH=$(PWD):$$PATH

EXPORT = $(shell $(DIRECTORY))

FILE=install.sh

EXPORTFILE = $(shell source $(FILE))

#PATH := $(PATH):$(PWD)
# PATH = $(shell pwd)/bin:$PATH
#@echo $(PATH)
#@echo $(shell export PATH=$(PATH))

all: EvalMain copy

EvalMain: $(evaluator)
	$(ARGS)

copy:
	@if [ ! -d $(BIN) ]; \
		then \
		mkdir $(BIN); \
	fi;
	@mv $(EXECUTABLE) $(BIN)
	@echo "DONE"
	@echo "Para ejecutar: ./$(BIN)/$(EXECUTABLE)"

clean:
	rm -rf $(BIN)
	rm -f /dev/shm/$(EXECUTABLE)
	@echo "DONE"
