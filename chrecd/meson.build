executable('chrecd',
  'interface/chre_slpi.c',
  'main.c',
  c_args : cflags,
  include_directories : include,
  install : true,
  link_with : libhexagonrpc,
  install_dir : client_target,
)
