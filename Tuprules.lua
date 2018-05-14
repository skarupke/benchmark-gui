FILE_ADD_FLAGS = {}
FILE_REMOVE_FLAGS = {}

function ivalues(table)
	local iterator, state, key = ipairs(table)
	local function viterator(state)
		local v
		key, v = iterator(state, key)
		return v
	end
	return viterator, state
end

function glob_cpp(folder)
	local result = tup.glob(folder .. '*.cpp')
	result += tup.glob(folder .. '*.cc')
	return result
end

function glob_shader(folder)
	local shader_files = tup.glob(folder .. '*.frag')
	shader_files += tup.glob(folder .. '*.vert')
	return shader_files
end

function copy_matrix(matrix)
	local result = {}
	for k, v in ipairs(matrix) do
		result[k] = v
	end
	return result
end

tup.include(tup.getconfig('COMPILER') .. '.lua')
tup.include(tup.getconfig('VARIANT') .. '.lua')
build_options = tup.getconfig('BUILD_OPTIONS')
if build_options != '' then
	tup.include(build_options .. '.lua')
end

WARNING_FLAGS += '-Werror'
WARNING_FLAGS += '-Wall'
WARNING_FLAGS += '-Wextra'
WARNING_FLAGS += '-Wno-unknown-pragmas'
WARNING_FLAGS += '-Wno-unknown-attributes'
WARNING_FLAGS += '-Wdeprecated'

DEFINES += '__STDC_LIMIT_MACROS'
DEFINES += '__STDC_CONSTANT_MACROS'
DEFINES += 'BOOST_SYSTEM_NO_DEPRECATED'
DEFINES += 'BOOST_FILESYSTEM_NO_DEPRECATED'
DEFINES += 'BOOST_MULTI_INDEX_DISABLE_SERIALIZATION'
DEFINES += 'BOOST_DETAIL_NO_CONTAINER_FWD'
DEFINES += 'HAVE_STD_REGEX'
DEFINES += 'CORO_NO_EXCEPTIONS'
DEFINES += 'QT_NO_KEYWORDS'

--DEFINES += 'RUN_SLOW_TESTS'

CPP_FLAGS += '-std=c++17'

-- preprocess only
--CPP_FLAGS += '-E'

--CPP_FLAGS += '-fno-rtti'
--DEFINES += 'GTEST_HAS_RTTI=0'

CPP_LINKER = CPP_COMPILER

CPP_FLAGS += '-g'
CPP_FLAGS += '-fno-omit-frame-pointer'
CPP_FLAGS += '-fdebug-prefix-map=./src/=../src/'
CPP_FLAGS += '-fdebug-prefix-map=src/=../src/'
CPP_FLAGS += '-fdebug-prefix-map=./libs/=../libs/'
CPP_FLAGS += '-fdebug-prefix-map=libs/=../libs/'
CPP_FLAGS += '-fdebug-prefix-map=./main=../main'

CPP_OUTPUT_EXTENSIONS += '.o'

function compile_cpp(source, inputs)
	local outputs = {}
	for v in ivalues(CPP_OUTPUT_EXTENSIONS) do
		outputs += source .. v
	end
    inputs += source
	local flags = copy_matrix(CPP_FLAGS)
	flags += WARNING_FLAGS
	for v in ivalues(SYSTEM_INCLUDE_DIRS) do
		flags += '-isystem' .. v
	end
	for v in ivalues(INCLUDE_DIRS) do
		flags += '-I' .. v
	end
	for v in ivalues(DEFINES) do
		flags += '-D' .. v
	end
	local additive_flags = FILE_ADD_FLAGS[source]
	if additive_flags != nil then
		for f in ivalues(additive_flags) do
			flags += f
		end
	end
	local subtractive_flags = FILE_REMOVE_FLAGS[source]
	if subtractive_flags != nil then
		local new_flags = {}
		for v in ivalues(flags) do
			if subtractive_flags[v] == nil then
				new_flags += v
			end
		end
		flags = new_flags
	end
	tup.definerule{ inputs = inputs,
					command = CPP_COMPILER .. ' ' .. table.concat(flags, ' ') .. ' -c ' .. source .. ' -o ' .. source .. '.o',
					outputs = outputs }
end

