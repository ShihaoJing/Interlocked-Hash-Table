BITS = 32

# Output directory
ODIR  = obj32
tmp  := $(shell mkdir -p $(ODIR))

# Basic compiler configuration and flags
CXX      = g++
CXXFLAGS = -MMD -ggdb -O3 -std=gnu++11 -m$(BITS)
LDFLAGS	 = -m$(BITS) -lpthread

# The basenames of the c++ files that this program uses
CXXFILES = main MurmurHash3

# The executable we will build
TARGET = $(ODIR)/main

# Create the .o names from the CXXFILES
OFILES = $(patsubst %, $(ODIR)/%.o, $(CXXFILES))

# Create .d files to store dependency information, so that we don't need to
# clean every time before running make
DFILES = $(patsubst %.o, %.d, $(OFILES))

# Default rule builds the executable
all: $(TARGET)

# clean up everything by clobbering the output folder
clean:
	@echo cleaning up...
	@rm -rf $(ODIR)

# build an .o file from a .cc file
$(ODIR)/%.o: %.cpp
	@echo [CXX] $< "-->" $@
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

# Link rule for building the target from .o files
$(TARGET): $(OFILES)
	@echo [LD] $^ "-->" $@
	@$(CXX) -o $@ $^ $(LDFLAGS)

# Remember that 'all' and 'clean' aren't real targets
.PHONY: all clean

# Pull in all dependencies
-include $(DFILES)