CPP_FLAGS += '-O2'
DEFINES += 'NDEBUG'
DEFINES += 'BENCHMARK_OPTIMIZER_STRING=release-no-cmov-converter'

CPP_FLAGS += '-mllvm'
CPP_FLAGS += '-x86-cmov-converter=false'
