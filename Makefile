BUILD_DIR = build
LIBRARY_BUILD_DIR = $(BUILD_DIR)/library
TEST_BUILD_DIR = $(BUILD_DIR)/test
RELEASE_TGZ = $(BUILD_DIR)/release.tgz

CC = g++
CFLAGS = -I$(LIBRARY_BUILD_DIR) -fsanitize=address -static-libasan -DTEST -Wall -Wno-multichar -Werror

SOURCE_DIR = source
MAIN_SOURCE_DIR = $(SOURCE_DIR)/main
TEST_SOURCE_DIR = $(SOURCE_DIR)/test

LIBRARY_FILES = $(notdir $(wildcard $(MAIN_SOURCE_DIR)/*))
TEST_CPP_SOURCES = $(notdir $(wildcard $(MAIN_SOURCE_DIR)/*.cpp $(TEST_SOURCE_DIR)/*.cpp))

VPATH = $(MAIN_SOURCE_DIR) : $(TEST_SOURCE_DIR)

.PHONY: default library test release clean

default : release

library : $(LIBRARY_FILES:%=$(LIBRARY_BUILD_DIR)/%)

$(LIBRARY_BUILD_DIR)/% : % | $(LIBRARY_BUILD_DIR)
	cp $< $@

$(LIBRARY_BUILD_DIR) :
	mkdir -p $@

test : library $(TEST_BUILD_DIR)/a.out
	$(TEST_BUILD_DIR)/a.out
	
$(TEST_BUILD_DIR)/a.out : $(TEST_CPP_SOURCES:%.cpp=$(TEST_BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) -o $@ $^
	
$(TEST_BUILD_DIR)/%.o : %.cpp Makefile | $(TEST_BUILD_DIR)
	$(CC) -c $(CFLAGS) -MMD -o $@ $<
	
$(TEST_BUILD_DIR) :
	mkdir -p $@

release: library test $(RELEASE_TGZ)

$(RELEASE_TGZ) : $(LIBRARY_FILES:%=$(LIBRARY_BUILD_DIR)/%)
	tar -C $(BUILD_DIR) -cf $@ $(LIBRARY_FILES:%=library/%)

clean:
	@ rm -rf $(BUILD_DIR)

-include $(wildcard $(TEST_BUILD_DIR)/*.d)
	