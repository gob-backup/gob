cc = meson.get_compiler('c')

args = [
    '-Wno-long-long',
    '-Wno-padded',
    '-Wno-disabled-macro-expansion',
    '-D_POSIX_C_SOURCE=200809L',
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
  'gob',
  install: true,
  c_args: args,
  sources: [
      'gob.c',
      'cat.c',
      'chunk.c',
      'common.c',
      'fsck.c',
      'init.c',
      'blake2/blake2b-ref.c',
      config
  ],
)
