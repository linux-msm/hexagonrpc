project('fastrpc', 'c', version: '0.3.2')

version_array = meson.project_version().split('.')
api_version = '.'.join([version_array[0], version_array[1]])

include = include_directories('include')
client_target = get_option('libexecdir') / 'hexagonrpc'

cflags = ['-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter']

if get_option('hexagonrpcd_verbose')
  cflags += '-DHEXAGONRPC_VERBOSE'
endif

subdir('libhexagonrpc')
subdir('hexagonrpcd')
subdir('chrecd')

subdir('tests')
