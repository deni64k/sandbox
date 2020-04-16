project(vectorization LANGUAGES CXX)

add_executable(simple_mat simple_mat.cxx)

# add_dependencies(simple_mat googletest googlebenchmark)

target_link_libraries(simple_mat
  PRIVATE Threads::Threads
  PRIVATE benchmark::benchmark
)
