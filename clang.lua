CPP_COMPILER = '/usr/bin/clang++-18'

--CPP_FLAGS += '-fno-exceptions'
--DEFINES += 'SIG_NO_EXCEPTIONS'
--DEFINES += 'BOOST_NO_EXCEPTIONS'

--CPP_FLAGS += '--analyze'
CPP_FLAGS += '-mllvm'
CPP_FLAGS += '-x86-cmov-converter=false'
