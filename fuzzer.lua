CPP_FLAGS += '-fsanitize=fuzzer-no-link,address'
LINK_FLAGS += '-fsanitize=fuzzer,address'
DEFINES += 'ADDRESS_SANITIZER_BUILD'
DEFINES += 'FUZZER_BUILD'
