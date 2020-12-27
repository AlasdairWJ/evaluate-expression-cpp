CC = cl /EHsc /nologo /W4 /wd4100

SOURCE = src/evaluator.cpp
OBJECTS = build/evaluator.obj

TEST_SOURCE = test/test.cpp
TEST_EXE = bin/test.exe
TEST_OBJECTS = build/test.obj

all: clean lib test

lib:
	$(CC) /Fo:$(OBJECTS) /c $(SOURCE) /I "include"

test: lib
	$(CC) /Fe:$(TEST_EXE) /Fo:$(TEST_OBJECTS) $(TEST_SOURCE) $(OBJECTS) /I "include" 

clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(TEST_EXE)