cc = meson.get_compiler('c')

args = [
  '-Wall', '-Wextra', '-pedantic', '-Wno-long-long',
  '-D_BSD_SOURCE', '-D_POSIX_C_SOURCE=200809L',
]

config_data = configuration_data()
config_data.set('VERSION', meson.project_version())
if cc.has_function('__fpending')
  config_data.set('HAVE_FPENDING', 1)
endif

config = configure_file(
    input: 'config.h.in',
    output: 'config.h',
    configuration: config_data
)

executable(
  'gob-cat',
  install: true,
  c_args: args,
  sources: [ 'gob-cat.c', 'common.c', 'blake2/blake2b-ref.c', config ],
)

executable(
  'gob-chunk',
  install: true,
  c_args: args,
  sources: [ 'gob-chunk.c', 'common.c', 'blake2/blake2b-ref.c', config ],
)

executable(
  'gob-fsck',
  install: true,
  c_args: args,
  sources: [ 'gob-fsck.c', 'common.c', 'blake2/blake2b-ref.c', config ],
)
