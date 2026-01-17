// highlight.js language definition for SharpScript
(function () {
  function sharpscript(hljs) {
    const KEYWORDS = [
      'function', 'return', 'break', 'continue',
      'if', 'else', 'while', 'for', 'void', 'end',
      'true', 'false', 'null'
    ];
    const BUILTINS = [
      'system.print', 'system.input', 'system.len', 'system.type',
      'system.output', 'system.error', 'system.warning'
    ];
    return {
      name: 'SharpScript',
      keywords: {
        keyword: KEYWORDS.join(' '),
        built_in: BUILTINS.join(' ')
      },
      contains: [
        hljs.C_LINE_COMMENT_MODE, // support # comments as separate pattern
        {
          className: 'comment',
          begin: /#.*$/,
          relevance: 0
        },
        hljs.QUOTE_STRING_MODE,
        hljs.NUMBER_MODE,
        {
          className: 'symbol',
          match: /&insert/
        },
        {
          className: 'operator',
          match: /=>|==|!=|<=|>=|\+|\-|\*|\/|%|&&|\|\|/
        },
        {
          className: 'punctuation',
          match: /[{}\[\]\(\),.;]/
        }
      ]
    };
  }
  if (typeof window !== 'undefined' && window.hljs) {
    window.hljs.registerLanguage('sharpscript', sharpscript);
  } else if (typeof module !== 'undefined') {
    module.exports = function (hljs) { return sharpscript(hljs); };
  }
})();
