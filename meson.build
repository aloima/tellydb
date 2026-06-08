project('tellydb', 'c',
  version: '1.0.0',
  default_options: [
    'c_std=c23',
    'warning_level=2',
    'werror=false',
  ]
)

cc = meson.get_compiler('c')

git = find_program('git', required: false)
git_hash = 'unknown'
git_version = '0.1.0'

if git.found()
  # Retrieve git commit hash
  r_hash = run_command(git, 'rev-parse', 'HEAD', check: false)
  if r_hash.returncode() == 0
    git_hash = r_hash.stdout().strip()
  endif

  # Retrieve git version (latest tag)
  r_ver = run_command(git, 'describe', '--abbrev=0', '--tags', check: false)
  if r_ver.returncode() == 0
    git_version = r_ver.stdout().strip()
  endif
endif

add_project_arguments(
  '-DGIT_HASH="' + git_hash + '"',
  '-DVERSION="' + git_version + '"',
  '-D_TIME_BITS=64',
  '-D_GNU_SOURCE',
  '-D_FILE_OFFSET_BITS=64',
  language: 'c'
)

inc_dirs = include_directories('headers')

deps = [
  dependency('threads',  required: true),
  dependency('openssl',  required: true),
  dependency('gmp',      required: true),
  dependency('jemalloc', required: true),
]

if host_machine.system() != 'darwin'
  deps += cc.find_library('m', required: true)
endif

link_args = []
if host_machine.system() == 'darwin'
  link_args += '-Wl,-bind_at_load'
else
  link_args += '-Wl,-z,now'
endif

gperf = find_program('gperf', required: true)
command_hash_c = custom_target('command_hash.c',
  input: 'src/utils/command_hash.gperf',
  output: 'command_hash.c',
  command: [gperf, '-L', 'ANSI-C', '--compare-strncmp', '--ignore-case', '@INPUT@', '--output-file', '@OUTPUT@']
)

aligned_c_args = []
if cc.has_argument('-mavx2')
  aligned_c_args += '-mavx2'
elif cc.has_argument('-msse2')
  aligned_c_args += '-msse2'
endif

aligned_utils = static_library('aligned_utils',
  sources: [
    'src/utils/memcpy_aligned.c',
    'src/utils/memset_aligned.c',
  ],
  include_directories: inc_dirs,
  c_args: aligned_c_args,
  dependencies: deps
)

# Source files list
sources = [
  'src/telly.c',

  'src/commands/database/bgsave.c',
  'src/commands/database/dbsize.c',
  'src/commands/database/flushall.c',
  'src/commands/database/flushdb.c',
  'src/commands/database/lastsave.c',
  'src/commands/database/save.c',
  'src/commands/database/select.c',

  'src/commands/generic/age.c',
  'src/commands/generic/auth.c',
  'src/commands/generic/client.c',
  'src/commands/generic/command.c',
  'src/commands/generic/discard.c',
  'src/commands/generic/echo.c',
  'src/commands/generic/exec.c',
  'src/commands/generic/hello.c',
  'src/commands/generic/info.c',
  'src/commands/generic/multi.c',
  'src/commands/generic/ping.c',
  'src/commands/generic/pwd.c',
  'src/commands/generic/time.c',

  'src/commands/hashtable/hdel.c',
  'src/commands/hashtable/hget.c',
  'src/commands/hashtable/hgetall.c',
  'src/commands/hashtable/hkeys.c',
  'src/commands/hashtable/hlen.c',
  'src/commands/hashtable/hset.c',
  'src/commands/hashtable/htype.c',
  'src/commands/hashtable/hvals.c',

  'src/commands/kv/append.c',
  'src/commands/kv/decr.c',
  'src/commands/kv/decrby.c',
  'src/commands/kv/del.c',
  'src/commands/kv/exists.c',
  'src/commands/kv/get.c',
  'src/commands/kv/incr.c',
  'src/commands/kv/incrby.c',
  'src/commands/kv/rename.c',
  'src/commands/kv/set.c',
  'src/commands/kv/type.c',

  'src/commands/list/lindex.c',
  'src/commands/list/llen.c',
  'src/commands/list/lpop.c',
  'src/commands/list/lpush.c',
  'src/commands/list/ltype.c',
  'src/commands/list/rpop.c',
  'src/commands/list/rpush.c',

  'src/database/databases.c',
  'src/database/delete.c',
  'src/database/file/file.c',
  'src/database/file/generate.c',
  'src/database/file/size.c',
  'src/database/get.c',
  'src/database/kv.c',
  'src/database/read.c',
  'src/database/set.c',

  'src/resp/data.c',
  'src/resp/inline.c',
  'src/resp/resp.c',
  'src/resp/utils.c',

  'src/server/auth.c',
  'src/server/client.c',
  'src/server/commands.c',
  'src/server/events.c',
  'src/server/io.c',
  'src/server/password.c',
  'src/server/read_command.c',
  'src/server/server.c',
  'src/server/utils.c',

  'src/transactions/thread.c',
  'src/transactions/transactions.c',
  'src/transactions/variables.c',

  'src/utils/arena.c',
  'src/utils/config.c',
  'src/utils/file.c',
  'src/utils/hashset.c',
  'src/utils/hashtable.c',
  'src/utils/integer.c',
  'src/utils/linkedlist.c',
  'src/utils/logging.c',
  'src/utils/notifier.c',
  'src/utils/queue.c',
  'src/utils/string.c',
  'src/utils/tqueue.c',
  'src/utils/vector.c',

  command_hash_c,
]

telly_exe = executable('telly',
  sources: sources,
  include_directories: inc_dirs,
  dependencies: deps,
  link_with: aligned_utils,
  link_args: link_args,
  install: true
)

pytest = find_program('pytest', required: false)
python = find_program('python3', required: true)

if pytest.found()
  test('pytest-suite',
    python,
    args: [
      files('tests/meson_test_runner.py'),
      telly_exe
    ],
    timeout: 300, # 5 minutes timeout
    is_parallel: false
  )
endif
