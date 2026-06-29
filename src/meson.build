executable('prevue',
  sources      : files(
    'main.c',
    'image.c',
    'config.c',
    'minimap.c',
    'ipc.c',
  ),
  dependencies : deps,
  c_args       : ['-DPREVUE_DATADIR="' + (get_option('prefix') / get_option('datadir') / 'prevue') + '"'],
  install      : true,
  install_dir  : get_option('bindir'),
)
