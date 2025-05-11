CXX_FLAGS = -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations \
   -Wmissing-declarations -Wcast-qual -Wchar-subscripts                             \
   -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal           \
   -Wformat-nonliteral -Wformat-security -Wformat-signedness -Winline                   \
   -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked                     \
   -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo           \
   -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn                         \
   -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default               \
   -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast                    \
   -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing            \
   -Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new -fsized-deallocation             \
   -fstack-protector -fstrict-overflow -fno-omit-frame-pointer -Wlarger-than=8192                  \
   -Wstack-usage=8192 -fsanitize=address -fsanitize=undefined -fPIE -Werror=vla -Wno-format

REL_FLAGS = -O3

CXX = g++

back: ./bin/back.o ./bin/dump.o
	$(CXX) ./bin/back.o ./bin/dump.o $(CXXFLAGS) -o back

./bin/back.o:  src/x86back.cpp hpp/back.hpp
	$(CXX) -c ./src/x86back.cpp $(CXXFLAGS) -o ./bin/back.o

./bin/dump.o: src/dump.cpp hpp/dump.hpp hpp/ops.hpp
	$(CXX) -c ./src/dump.cpp $(CXXFLAGS) -o ./bin/dump.o

clean:
	rm -f main back format ./bin/*.o ./bin/png/*.png
