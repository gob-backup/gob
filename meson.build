project(
  'gob',
  'c',
  version: '0.6',
  default_options: [ 'c_std=c89' ]
)

args = [
  '-Wall', '-Wextra', '-pedantic',
  '-D_BSD_SOURCE', '-D_POSIX_C_SOURCE=200809L', '-DGOB_VERSION="@0@"'.format(meson.project_version())
]
foreach arg : args
  add_project_arguments(arg, language: 'c')
endforeach

libsodium = dependency('libsodium')

executable(
  'gob-cat',
  install: true,
  sources: [ 'gob-cat.c', 'common.c' ],
  dependencies: libsodium
)

executable(
  'gob-chunk',
  install: true,
  sources: [ 'gob-chunk.c', 'common.c' ],
  dependencies: libsodium
)

executable(
  'gob-fsck',
  install: true,
  sources: [ 'gob-fsck.c', 'common.c' ],
  dependencies: libsodium
)

tests = find_program('test.sh')
test('gob', tests)
