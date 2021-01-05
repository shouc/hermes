/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstring>
#include <hermes/hermes.h>
#include <jsi/jsi.h>
using facebook::hermes::HermesRuntime;
using facebook::hermes::makeHermesRuntime;
using facebook::jsi::HostObject;
using facebook::jsi::JSIException;
using facebook::jsi::Object;
using facebook::jsi::PropNameID;
using facebook::jsi::Runtime;
using facebook::jsi::String;
using facebook::jsi::StringBuffer;
using facebook::jsi::Value;

//
// BEGIN FUZZING CODE
//
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#define REPRL_CRFD 100
#define REPRL_CWFD 101
#define REPRL_DRFD 102
#define REPRL_DWFD 103

#define SHM_SIZE 0x100000
#define MAX_EDGES ((SHM_SIZE - 4) * 8)

#define CHECK(cond) if (!(cond)) { fprintf(stderr, "\"" #cond "\" failed\n"); _exit(-1); }

struct shmem_data {
  uint32_t num_edges;
  unsigned char edges[];
};

struct shmem_data* __shmem;

uint32_t *__edges_start, *__edges_stop;
void __sanitizer_cov_reset_edgeguards() {
  uint64_t N = 0;
  for (uint32_t *x = __edges_start; x < __edges_stop && N < MAX_EDGES; x++)
    *x = ++N;
}

extern "C" void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop) {
  // Avoid duplicate initialization
  if (start == stop || *start)
    return;

  if (__edges_start != NULL || __edges_stop != NULL) {
    fprintf(stderr, "Coverage instrumentation is only supported for a single module\n");
    _exit(-1);
  }

  __edges_start = start;
  __edges_stop = stop;

  // Map the shared memory region
  const char* shm_key = getenv("SHM_ID");
  if (!shm_key) {
    puts("[COV] no shared memory bitmap available, skipping");
    __shmem = (struct shmem_data*) malloc(SHM_SIZE);
  } else {
    int fd = shm_open(shm_key, O_RDWR, S_IREAD | S_IWRITE);
    if (fd <= -1) {
      fprintf(stderr, "Failed to open shared memory region: %s\n", strerror(errno));
      _exit(-1);
    }

    __shmem = (struct shmem_data*) mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (__shmem == MAP_FAILED) {
      fprintf(stderr, "Failed to mmap shared memory region\n");
      _exit(-1);
    }
  }

  __sanitizer_cov_reset_edgeguards();

  __shmem->num_edges = stop - start;
  printf("[COV] edge counters initialized. Shared memory: %s with %u edges\n", shm_key, __shmem->num_edges);
}

extern "C" void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
  // There's a small race condition here: if this function executes in two threads for the same
  // edge at the same time, the first thread might disable the edge (by setting the guard to zero)
  // before the second thread fetches the guard value (and thus the index). However, our
  // instrumentation ignores the first edge (see libcoverage.c) and so the race is unproblematic.
  uint32_t index = *guard;
  // If this function is called before coverage instrumentation is properly initialized we want to return early.
  if (!index) return;
  __shmem->edges[index / 8] |= 1 << (index % 8);
  *guard = 0;
}

//
// END FUZZING CODE
//

int main(int argc, char** argv){

  if (argc == 2 && strcmp(argv[1], "--replr") == 0){
    // replr mode
    auto runtime = makeHermesRuntime();

    // get fuzzer arguments
    char helo[] = "HELO";
    if (write(REPRL_CWFD, helo, 4) != 4 || read(REPRL_CRFD, helo, 4) != 4) {
      printf("Invalid HELO response from parent\n");
      exit(-1);
    }

    if (memcmp(helo, "HELO", 4) != 0) {
      printf("Invalid response from parent\n");
      exit(-1);
    }
    while (true){
      size_t script_size = 0;
      unsigned action;
      CHECK(read(REPRL_CRFD, &action, 4) == 4);
      if (action == 'cexe') {
        CHECK(read(REPRL_CRFD, &script_size, 8) == 8);
      } else {
        printf("Unknown Action\n");
        exit(-1);
      }
      char *script_src = (char *)(malloc(script_size+1));
      char *ptr = script_src;
      size_t remaining = script_size;
      while (remaining > 0) {
        ssize_t rv = read(REPRL_DRFD, ptr, remaining);
        if (rv <= 0) {
          fprintf(stderr, "Failed to load script\n");
          exit(-1);
        }
        remaining -= rv;
        ptr += rv;
      }
      script_src[script_size] = '\0';
      std::string code(script_src);
      free(script_src);
      bool exceptionThrew(false);
      try {
        runtime->evaluateJavaScript(std::make_unique<StringBuffer>(code), "");
      } catch (const JSIException &e) {
        exceptionThrew = true;
      }
      fflush(stdout);
      fflush(stderr);
      auto status = ((exceptionThrew ? 1 : 0) & 0xff) << 8;
      CHECK(write(REPRL_CWFD, &status, 4) == 4);
      __sanitizer_cov_reset_edgeguards();
    }

  } else {
    // peacefully quit
    return 0;
  }
}

