var SpellCheck = require('./');

spell = new SpellCheck('/usr/share/hunspell/en_US.aff', '/usr/share/hunspell/en_US.dic');

spell.check('sain', function (err, correct, suggestions) {
	console.log(arguments);
});
