{
  'targets': [
    {
      'target_name': 'spellcheck',
      'sources': [
        'src/spellcheck.cc',
      ],
      'cflags': [ '-O3' ],
      'dependencies': [
        'deps/hunspell/binding.gyp:hunspell',
      ],
    },
  ],
}