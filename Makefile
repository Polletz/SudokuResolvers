ifndef FF_ROOT 
FF_ROOT		= ${HOME}/Desktop/fastflow
endif

CXX		= g++ -std=c++17
INCLUDES	= -I $(FF_ROOT) 
CXXFLAGS  	= #-ftree-vectorize -fopt-info-vec-missed # -g -DNO_DEFAULT_MAPPING -DBLOCKING_MODE -DFF_BOUNDED_BUFFER

LDFLAGS 	= -pthread
OPTFLAGS	= -O3

TARGETS		= 	Sudoku-seq-BF	\
				Sudoku-DC \
				Sudoku-FF \
				Sudoku-single-queue


.PHONY: all clean cleanall
.SUFFIXES: .cpp 


%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o ./build/$@ $< $(LDFLAGS)

all		: $(TARGETS)
clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~
