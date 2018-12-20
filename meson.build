project(
  'gob',
  'c',
  version: '0.6',
  default_options: [ 'c_std=c89' ]
)

subdir('src')

tests = find_program('test.sh')
test('gob', tests)
