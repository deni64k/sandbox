add_executable(simple_mat simple_mat.cxx)

add_dependencies(simple_mat googletest googlebenchmark)

target_link_libraries(simple_mat
  LINK_PRIVATE Threads::Threads
  LINK_PRIVATE benchmark::benchmark
)
