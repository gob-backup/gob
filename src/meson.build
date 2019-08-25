cc = meson.get_compiler('c')

args = [
  '-Wall', '-Wextra', '-pedantic', '-Wno-long-long',
  '-D_BSD_SOURCE', '-D_POSIX_C_SOURCE=200809L', '-DGOB_VERSION="@0@"'.format(meson.project_version())
]
foreach arg : args
  add_project_arguments(arg, language: 'c')
endforeach

if cc.has_function('__fpending')
  add_project_arguments('-DHAVE_FPENDING', language: 'c')
endif

executable(
  'gob-cat',
  install: true,
  sources: [ 'gob-cat.c', 'common.c', 'blake2/blake2b-ref.c' ]
)

executable(
  'gob-chunk',
  install: true,
  sources: [ 'gob-chunk.c', 'common.c', 'blake2/blake2b-ref.c' ]
)

executable(
  'gob-fsck',
  install: true,
  sources: [ 'gob-fsck.c', 'common.c', 'blake2/blake2b-ref.c' ]
)
