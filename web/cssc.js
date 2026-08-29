/* cssclang.com

   Two small things happen here, both of them so the markup can stay plain.

   1. Code blocks are written as ordinary CSSC in the HTML and colored at load
      with the same palette the IDE uses. To add a snippet you paste the code
      into <pre><code class="cssc"> and nothing else. Without JavaScript the
      code is still there, just in one color.

   2. The mail address is assembled from parts, so the harvesters that scrape
      pages for anything shaped like an address come up empty. */

(function () {
  "use strict";

  var KEYWORDS = [
    "select", "jump", "if", "else", "for", "while", "return", "mirror",
    "call", "destruct", "break", "continue", "object", "sector", "free",
    "public", "private", "true", "false", "null", "none", "in"
  ];

  var TYPES = [
    "int", "float", "string", "bool", "auto", "var", "void",
    "vector", "array", "map", "bind", "list", "matrix"
  ];

  /* One pass, longest and most specific first: docstrings before comments,
     comments before everything (a // inside a string is handled because the
     string alternative sits above the bare identifier rules and the scanner
     never re-enters consumed text). */
  var TOKEN = new RegExp([
    "(\\/\\/d[^\\n]*)",                                  // 1 docstring
    "(\\/\\/[^\\n]*|\\/\\*[\\s\\S]*?\\*\\/)",            // 2 comment
    "(\"(?:[^\"\\\\]|\\\\.)*\"|'(?:[^'\\\\]|\\\\.)*')",  // 3 string
    "(#[A-Za-z_][A-Za-z0-9_]*)",                         // 4 directive
    "(0x[0-9A-Fa-f]+|0b[01]+|\\d+(?:\\.\\d+)?)",         // 5 number
    "([A-Za-z_][A-Za-z0-9_]*)",                          // 6 identifier
    "([&*](?=[A-Za-z_]))"                                // 7 reference marker
  ].join("|"), "g");

  function escapeHtml(text) {
    return text
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;");
  }

  function span(cls, text) {
    return '<span class="tok-' + cls + '">' + escapeHtml(text) + "</span>";
  }

  function classifyIdentifier(name, source, after) {
    if (KEYWORDS.indexOf(name) !== -1) return "keyword";
    if (TYPES.indexOf(name) !== -1) return "type";

    var rest = source.slice(after);
    if (/^\s*::/.test(rest)) return "ns";
    if (/^\s*\(/.test(rest)) return "func";
    return null;
  }

  function highlight(source) {
    var out = "";
    var last = 0;
    var match;

    TOKEN.lastIndex = 0;
    while ((match = TOKEN.exec(source)) !== null) {
      out += escapeHtml(source.slice(last, match.index));
      last = TOKEN.lastIndex;

      if (match[1]) {
        out += span("doc", match[1]);
      } else if (match[2]) {
        out += span("comment", match[2]);
      } else if (match[3]) {
        out += span("string", match[3]);
      } else if (match[4]) {
        out += span("directive", match[4]);
      } else if (match[5]) {
        out += span("number", match[5]);
      } else if (match[6]) {
        var cls = classifyIdentifier(match[6], source, last);
        out += cls ? span(cls, match[6]) : escapeHtml(match[6]);
      } else if (match[7]) {
        out += span("marker", match[7]);
      }
    }

    return out + escapeHtml(source.slice(last));
  }

  var blocks = document.querySelectorAll("pre code.cssc");
  for (var i = 0; i < blocks.length; i++) {
    blocks[i].innerHTML = highlight(blocks[i].textContent);
  }

  var mails = document.querySelectorAll(".mail");
  for (var j = 0; j < mails.length; j++) {
    var el = mails[j];
    var address = el.getAttribute("data-user") +
      String.fromCharCode(64) +
      el.getAttribute("data-host");
    var link = document.createElement("a");
    link.href = "mailto:" + address;
    link.textContent = address;
    el.parentNode.replaceChild(link, el);
  }
})();
