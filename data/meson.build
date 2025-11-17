systemd_units = [
  'hexagonrpcd-adsp-rootpd.service',
  'hexagonrpcd-adsp-sensorspd.service',
  'hexagonrpcd-sdsp.service',
]

foreach unit : systemd_units
  configure_file(
    input: unit + '.in',
    output: unit,
    configuration: { 'bindir': get_option('prefix') / get_option('bindir') },
    install: true,
    install_dir: get_option('libdir') / 'systemd/system',
  )
endforeach
