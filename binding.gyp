{
  'targets': [
    {
      'target_name': 'spellcheck',
      'sources': [
        'spellcheck.cc',
      ],
      'cflags': [ '-O3' ],
      'dependencies': [
        'deps/hunspell/binding.gyp:hunspell',
      ],
    },
  ],
}