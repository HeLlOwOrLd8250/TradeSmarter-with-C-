CXX      = g++
CXXFLAGS = -std=c++17 -O3 -Wall

all: export_features.exe

export_features.exe: export_features.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	del /q export_features.exe 2>nul || true
