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
BINDIR=./bin
DATA_DIR:=$(shell pwd)/data

LIBRARY_NAME := libecs34proj3

INCLUDE  += -I $(INCLUDE_DIR)
DEFINES  += -DDATA_DIR=$(DATA_DIR)
CFLAGS   += -Wall -Werror -Wpedantic -fPIC
ifeq ($(RELEASE),1)
CFLAGS   += -O2
else
CFLAGS   += -g -O0
endif
CXXFLAGS += $(CFLAGS) -std=c++17
LDFLAGS  += -fuse-ld=$(LD) -lexpat

TEST_CFLAGS   := -Wall -Werror -Wpedantic -g -Og --coverage
TEST_CXXFLAGS := $(TEST_CFLAGS) -std=c++17
TEST_LDFLAGS  := -fuse-ld=$(LD) -lgcov -lpthread -lgtest -lgtest_main -lexpat

SOURCES := StringUtils StringDataSink StringDataSource DSVWriter DSVReader XMLReader XMLWriter CSVBusSystem OpenStreetMap GeographicUtils KMLWriter FileDataFactory FileDataSource StandardDataSource FileDataSink StandardErrorDataSink StandardDataSink StringDataSource StringDataSink
BINARIES := speedtest kmlout
TESTS := StringUtilsTest StringDataSinkTest StringDataSourceTest DSVTest XMLTest CSVBusSystemTest OpenStreetMapTest

.PHONY: all directories test_directories test clean build_test

all: directories binaries library test

build_test: test_directories $(addprefix $(TESTBINDIR)/, $(TESTS))

test: build_test
	for test in $(addprefix $(TESTBINDIR)/, $(TESTS)); do ./$$test; done
	lcov --capture --directory . --output-file $(COVERAGEDIR)/coverage.info --ignore-errors inconsistent,source,mismatch
	lcov --remove $(COVERAGEDIR)/coverage.info '/usr/*' '*/testsrc/*' --output-file $(COVERAGEDIR)/coverage.info
	genhtml $(COVERAGEDIR)/coverage.info --output-directory $(COVERAGEDIR)

library: $(addprefix $(OBJDIR)/, $(addsuffix .o, $(SOURCES)))
	$(CXX) $(CXXFLAGS) $(DEFINES) -shared $^ -o $(LIBDIR)/$(LIBRARY_NAME).so $(LDFLAGS)

$(BINDIR)/speedtest: $(OBJDIR)/Speedtest.o $(addprefix $(OBJDIR)/, $(addsuffix .o, $(SOURCES)))
	$(CXX) $(CXXFLAGS) $(DEFINES) $^ -o $@ $(LDFLAGS) -lpthread

$(BINDIR)/kmlout: $(OBJDIR)/KMLOut.o $(addprefix $(OBJDIR)/, $(addsuffix .o, $(SOURCES)))
	$(CXX) $(CXXFLAGS) $(DEFINES) $^ -o $@ $(LDFLAGS) -lpthread

binaries: $(addprefix $(BINDIR)/, $(BINARIES))

$(OBJDIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDE) -c $< -o $@

$(TESTOBJDIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CXX) $(TEST_CXXFLAGS) $(DEFINES) $(INCLUDE) -c $< -o $@

$(TESTOBJDIR)/%.o: $(TESTSOURCE_DIR)/%.cpp
	$(CXX) $(TEST_CXXFLAGS) $(DEFINES) $(INCLUDE) -c $< -o $@

$(addprefix $(TESTBINDIR)/, $(TESTS)): $(TESTBINDIR)/%: $(TESTOBJDIR)/%.o $(addprefix $(TESTOBJDIR)/, $(addsuffix .o, $(SOURCES)))
	$(CXX) $(TEST_CXXFLAGS) $(DEFINES) $^ -o $@ $(TEST_LDFLAGS)

directories: $(OBJDIR) $(LIBDIR) $(BINDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

test_directories: $(TESTBINDIR) $(TESTOBJDIR) $(COVERAGEDIR)

$(TESTOBJDIR):
	mkdir -p $(TESTOBJDIR)

$(TESTBINDIR):
	mkdir -p $(TESTBINDIR)

$(COVERAGEDIR):
	mkdir -p $(COVERAGEDIR)

clean:
	rm -rf $(OBJDIR) $(TESTOBJDIR) $(TESTBINDIR) $(COVERAGEDIR) $(LIBDIR) $(BINDIR)
