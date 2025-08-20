json_c = dependency('json-c', required : false)

if json_c.found()
  executable('sscregistrygen',
    'sscregistrygen.c',
    c_args : cflags,
    dependencies : [json_c])
endif
