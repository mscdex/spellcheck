Description
===========

An async hunspell binding for [node.js](http://nodejs.org/).


Requirements
============

* [node.js](http://nodejs.org/) -- v0.6.0 or newer
* [Dictionary and affix files](http://wiki.services.openoffice.org/wiki/Dictionaries)


Install
============

npm install spellcheck


Examples
========

* Check a word:
```javascript
  // this example uses the en_US hunspell files from SCOWL:
  //   http://wordlist.sourceforge.net/
  var SpellCheck = require('spellcheck'),
        base = __dirname + (process.platform === 'win32' ? '\\' : '/'),
        spell = new SpellCheck(base + 'en_US.aff', base + 'en_US.dic');

  spell.check('sain', function(err, correct, suggestions) {
      if (err) throw err;
      if (correct)
        console.log('Word is spelled correctly!');
      else
        console.log('Word not recognized. Suggestions: ' + suggestions);
  });

  // output:
  // Word not recognized. Suggestions: chain,sin,saint,satin,stain,slain,swain,rain,sail,lain,said,gain,main,spin,pain
```


API
===

Methods
-------

* **(constructor)**(<_String_>affixPath, <_Integer_>dictPath) - Creates and returns a new SpellCheck instance. affixPath is an **absolute** path that points to an affix (.aff) file. dictPath is an **absolute** path that points to a dictionary (.dic) file.

* **check**(<_String_>word, <_Function_>callback) - _(void)_ - Spell checks the given word. The callback receives three arguments: an <_Error_> object in case of error (null otherwise), a <_Boolean_> indicating if the word was spelled correctly, and if the word was not recognized, an <_Array_> of suggested words.
