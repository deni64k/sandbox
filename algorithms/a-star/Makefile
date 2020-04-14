CC?=gcc-9
CXX?=g++-9
LDFLAGS=-L/usr/local/lib $(CXXFLAGS)
CPPFLAGS=
BENCHMARK_CPPFLAGS=-isystem ../../vendor/google-benchmark/include
OPTFLAGS=-O3 -march=native -ggdb -fno-omit-frame-pointer
CXXFLAGS=-std=c++2a -fno-rtti -flto -Wall -Werror -pedantic $(OPTFLAGS)
BENCHMARK_LDFLAGS=../../vendor/google-benchmark/build/src/libbenchmark.a \
                  -lpthread
PERF_LDFLAGS= # -lprofiler

main: main.o astar.o
	$(CXX) $(LDFLAGS) -o $@ $^ -flto

bench: bench.o astar.o
	$(CXX) $(LDFLAGS) -o $@ $^ \
	       $(BENCHMARK_LDFLAGS) \
	       $(PERF_LDFLAGS)

%.o: %.cxx
	$(CXX) $(CPPFLAGS) $(BENCHMARK_CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	-@rm -f main.o bench.o astar.o
