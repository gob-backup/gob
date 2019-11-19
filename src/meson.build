cc = meson.get_compiler('c')

args = [
  '-Wall', '-Wextra', '-pedantic', '-Wno-long-long',
  '-D_BSD_SOURCE', '-D_POSIX_C_SOURCE=200809L', '-DGOB_VERSION="@0@"'.format(meson.project_version())
]

if cc.has_function('__fpending')
  args += '-DHAVE_FPENDING'
endif

executable(
  'gob-cat',
  install: true,
  c_args: args,
  sources: [ 'gob-cat.c', 'common.c', 'blake2/blake2b-ref.c' ]
)

executable(
  'gob-chunk',
  install: true,
  c_args: args,
  sources: [ 'gob-chunk.c', 'common.c', 'blake2/blake2b-ref.c' ]
)

executable(
  'gob-fsck',
  install: true,
  c_args: args,
  sources: [ 'gob-fsck.c', 'common.c', 'blake2/blake2b-ref.c' ]
)
