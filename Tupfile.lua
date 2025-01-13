
COPPERSPICE_DIRECTORY = tup.getcwd() .. '/libs/copperspice-1.4.4/'
COPPERSPICE_LIB_DIR = COPPERSPICE_DIRECTORY .. 'lib'

LINK_FLAGS += '-pthread'
LINK_FLAGS += '-fuse-ld=gold'
-- the next two lines are for incremental linking.
-- they are turned off because they seem to be slower
-- right now. maybe use it when the project gets bigger
--LINK_FLAGS += '-Wl,--incremental'
--LINK_FLAGS += '-fno-use-linker-plugin'
LINK_FLAGS += '-L' .. COPPERSPICE_LIB_DIR

LINK_FLAGS += '-Wl,-R'
LINK_FLAGS += '-Wl,' .. '../' .. COPPERSPICE_LIB_DIR
LINK_FLAGS += '-fPIE'
LINK_FLAGS += '-pie'
LINK_FLAGS += '-Wl,-pie'

LINK_LIBS += '-lCsGui1.4'
LINK_LIBS += '-lCsCore1.4'
LINK_LIBS += '-lsqlite3'
LINK_LIBS += '-lboost_system'
LINK_LIBS += '-lboost_thread'
LINK_LIBS += '-lstdc++fs'

INCLUDE_DIRS += tup.getcwd() .. '/src'
INCLUDE_DIRS += tup.getcwd() .. '/libs'
INCLUDE_DIRS += tup.getcwd() .. '/libs/benchmark/include'

SYSTEM_INCLUDE_DIRS += tup.getcwd() .. '/libs/gtest'
SYSTEM_INCLUDE_DIRS += COPPERSPICE_DIRECTORY .. 'include'
SYSTEM_INCLUDE_DIRS += COPPERSPICE_DIRECTORY .. 'include/QtCore'
SYSTEM_INCLUDE_DIRS += COPPERSPICE_DIRECTORY .. 'include/QtGui'

INPUT_FOLDERS += './'
INPUT_FOLDERS += 'src/'
INPUT_FOLDERS += 'src/container/'
INPUT_FOLDERS += 'src/custom_benchmark/'
INPUT_FOLDERS += 'src/db/'
INPUT_FOLDERS += 'src/debug/'
INPUT_FOLDERS += 'src/math/'
INPUT_FOLDERS += 'src/memory/'
INPUT_FOLDERS += 'src/signals/'
INPUT_FOLDERS += 'src/test/'
INPUT_FOLDERS += 'src/thread/'
INPUT_FOLDERS += 'src/util/'
INPUT_FOLDERS += 'libs/benchmark/src/'

EXECUTABLE_FOLDER = 'src/executables/'

mark_as_cachedir('src/')
mark_as_cachedir('libs/')

for v in ivalues(INPUT_FOLDERS) do
	CPP_SOURCES += glob_cpp(v)
	SHADER_SOURCES += glob_shader(v)
end
CPP_SOURCES += glob_cpp(EXECUTABLE_FOLDER)

OUTPUT_FOLDERS = INPUT_FOLDERS

CPP_SOURCES += 'libs/gtest/src/gtest-all.cc'
OUTPUT_FOLDERS += 'libs/gtest/src/'

for v in ivalues(CPP_SOURCES) do
    compile_cpp(v)
end

for v in ivalues(SHADER_SOURCES) do
	compile_shader(v)
end


executable_name = 'main'

objfiles = {}
for v in ivalues(OUTPUT_FOLDERS) do
    objfiles += tup.glob(v .. '*.o')
end
tup.definerule{ inputs = objfiles,
                command = CPP_LINKER .. ' ' .. table.concat(LINK_FLAGS, ' ') .. ' %f ' .. table.concat(LINK_LIBS, ' ') .. ' -o %o',
                outputs = {executable_name} }

for v in ivalues(tup.glob(EXECUTABLE_FOLDER .. "*.o")) do
    local objfiles
    objfiles = {}
    objfiles += v
    objfiles += tup.glob('src/custom_benchmark/*.o')
    objfiles += tup.glob('src/debug/*.o')
    objfiles += tup.glob('src/util/*.o')
    objfiles += tup.glob('libs/benchmark/src/*.o')
    objfiles += tup.glob('libs/gtest/src/*.o')
    tup.definerule{ inputs = objfiles,
                    command = CPP_LINKER .. ' ' .. table.concat(LINK_FLAGS, ' ') .. ' %f ' .. table.concat(LINK_LIBS, ' ') .. ' -o %o',
                    outputs = {tup.base(v) .. ".exe"} }
end
