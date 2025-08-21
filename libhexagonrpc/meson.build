libhexagonrpc = shared_library('hexagonrpc',
  'error.c',
  'handle.c',
  'hexagonrpc.c',
  'interface/remotectl.c',
  'session.c',
  c_args : cflags,
  include_directories : include,
  soversion : api_version,
  install : true
)

