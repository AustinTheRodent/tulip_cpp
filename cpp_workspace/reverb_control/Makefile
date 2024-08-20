SRC_DIR   := ./src
OBJ_DIR   := ./obj
EXE_DIR   := ./bin
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
LDFLAGS   := -lm
CPPFLAGS  := 
CXXFLAGS  := 

$(EXE_DIR)/main.exe: $(OBJ_FILES)
	g++ $(LDFLAGS) -o $@ $^ -lm

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(EXE_DIR)/*.exe
	rm -f $(OBJ_DIR)/*.o
