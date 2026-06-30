project('prevue', 'c', 
        version : '0.1.0')

efl_version = '>= 1.27.0'

cc = meson.get_compiler('c')

deps = [ dependency('ecore'),
         dependency('ecore-ipc', version : efl_version),
         dependency('evas', version : efl_version),
         dependency('edje', version : efl_version),
         dependency('elementary', version : efl_version),
         dependency('eet', version : efl_version),
         cc.find_library('m', required : true), ]

#### dir locations  
dir_prefix = get_option('prefix')
dir_bin    = join_paths(dir_prefix, get_option('bindir'))
dir_data   = join_paths(dir_prefix, get_option('datadir'))

edje_cc = find_program('edje_cc', required : true)

meson.add_install_script('sh', '-c',
  'xdg-icon-resource install --novendor --size 128 $MESON_INSTALL_DESTDIR_PREFIX/share/icons/hicolor/128x128/apps/prevue.png prevue')

subdir('src')
subdir('data')