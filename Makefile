CC=gcc
CXX=g++
LD=bfd

INCLUDE_DIR=./include
SOURCE_DIR=./src
TESTSOURCE_DIR=./testsrc
OBJDIR=./obj
LIBDIR=./lib
TESTOBJDIR=./testobj
TESTBINDIR=./testbin
COVERAGEDIR=./htmlcov

LIBRARY_NAME := libecs34proj3

INCLUDE  += -I $(INCLUDE_DIR)
DEFINES  +=
CFLAGS   += -Wall -Werror -Wpedantic -fPIC
ifeq ($(RELEASE),1)
CFLAGS   += -O2
else
CFLAGS   += -g -O0
endif
CXXFLAGS += $(CFLAGS) -std=c++23
LDFLAGS  += -fuse-ld=$(LD) -lexpat

TEST_CFLAGS   := -Wall -Werror -Wpedantic -g -O0 --coverage
TEST_CXXFLAGS := $(TEST_CFLAGS) -std=c++23
TEST_LDFLAGS  := -fuse-ld=$(LD) -lgcov -lpthread -lgtest -lgtest_main -lexpat

SOURCES := StringUtils StringDataSink StringDataSource DSVWriter DSVReader XMLReader XMLWriter CSVBusSystem OpenStreetMap
TESTS := StringUtilsTest StringDataSinkTest StringDataSourceTest DSVTest XMLTest OpenStreetMapTest

.PHONY: all directories test_directories test clean

all: directories library test

test: test_directories cover

cover: $(addprefix $(TESTBINDIR)/, $(TESTS))
	for test in $^; do ./$$test; done
	lcov --capture --directory . --output-file $(COVERAGEDIR)/coverage.info --ignore-errors inconsistent,source
	lcov --remove $(COVERAGEDIR)/coverage.info '/usr/*' '*/testsrc/*' --output-file $(COVERAGEDIR)/coverage.info
	genhtml $(COVERAGEDIR)/coverage.info --output-directory $(COVERAGEDIR)

library: $(addprefix $(OBJDIR)/, $(addsuffix .o, $(SOURCES)))
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(DEFINES) -shared $^ -o $(LIBDIR)/$(LIBRARY_NAME).so

$(OBJDIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDE) -c $< -o $@

$(TESTOBJDIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CXX) $(TEST_CXXFLAGS) $(DEFINES) $(INCLUDE) -c $< -o $@

$(TESTOBJDIR)/%.o: $(TESTSOURCE_DIR)/%.cpp
	$(CXX) $(TEST_CXXFLAGS) $(DEFINES) $(INCLUDE) -c $< -o $@

$(addprefix $(TESTBINDIR)/, $(TESTS)): $(TESTBINDIR)/%: $(TESTOBJDIR)/%.o $(addprefix $(TESTOBJDIR)/, $(addsuffix .o, $(SOURCES)))
	$(CXX) $(TEST_LDFLAGS) $(TEST_CXXFLAGS) $(DEFINES) $^ -o $@

directories: $(OBJDIR) $(LIBDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)

test_directories: $(TESTBINDIR) $(TESTOBJDIR) $(COVERAGEDIR)

$(TESTOBJDIR):
	mkdir -p $(TESTOBJDIR)

$(TESTBINDIR):
	mkdir -p $(TESTBINDIR)

$(COVERAGEDIR):
	mkdir -p $(COVERAGEDIR)

clean:
	rm -rf $(OBJDIR) $(TESTOBJDIR) $(TESTBINDIR) $(COVERAGEDIR) $(LIBDIR)
