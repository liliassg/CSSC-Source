#!/usr/bin/env python3
"""
Render the language reference in doc/ into web/docs/.

doc/ stays the source of truth and stays plain markdown, so the reference can
be read on GitHub, in an editor, or offline exactly as before. This only builds
a second view of it that matches the rest of the site.

Run it after editing anything in doc/:

    python3 webtools/make_docs.py

Chapter files become web/docs/<name>.html and doc/README.md becomes the index
at web/docs/index.html. Links between chapters are rewritten from .md to .html,
so they keep working in both places.
"""

import html
import re
from pathlib import Path

import markdown

HERE = Path(__file__).resolve().parent
DOCS = HERE.parent / "doc"
OUT = HERE.parent / "web" / "docs"

# Titles for the browser tab and the card when a chapter gets shared. Taken
# from the first heading of each file, with the number kept off.
TITLE_RE = re.compile(r"^#\s+(.+)$", re.M)


def nav(current):
    """The same toolbar as the rest of the site, one level down."""
    items = [
        ("../index.html", "main"),
        ("../ide.html", "ide"),
        ("../news.html", "news"),
        ("index.html", "docs"),
        ("https://github.com/liliassg/CSSC-Source", "github"),
    ]
    out = []
    for href, label in items:
        mark = ' aria-current="page"' if label == current else ""
        out.append(f'        <li><a href="{href}"{mark}>{label}</a></li>')
    return "\n".join(out)


def page(title, description, body, prev_link, next_link):
    """Wrap rendered markdown in the site shell."""
    around = ""
    if prev_link or next_link:
        parts = []
        if prev_link:
            parts.append(f'<a href="{prev_link[0]}">&lt; {prev_link[1]}</a>')
        if next_link:
            parts.append(f'<a href="{next_link[0]}">{next_link[1]} &gt;</a>')
        around = '\n    <nav class="chapter-nav">' + " ".join(parts) + "</nav>"

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)}</title>
<meta name="description" content="{html.escape(description)}">
<meta property="og:type" content="website">
<meta property="og:site_name" content="CSSC">
<meta property="og:title" content="{html.escape(title)}">
<meta property="og:description" content="{html.escape(description)}">
<meta property="og:image" content="https://cssclang.com/og.png">
<meta property="og:image:width" content="1200">
<meta property="og:image:height" content="630">
<meta name="twitter:card" content="summary_large_image">
<link rel="icon" href="../star.png" type="image/png">
<link rel="stylesheet" href="../font.css">
<link rel="stylesheet" href="../style.css">
</head>
<body>

<div class="page">

  <header class="masthead">
    <h1 class="wordmark"><a href="../index.html">CSSC</a></h1>
    <p class="expansion">Control Specified Source Compiling</p>
    <img class="star" src="../star.png" alt="" width="160" height="160">

    <nav class="toolbar" aria-label="Sections">
      <ul>
{nav("docs")}
      </ul>
    </nav>
    <hr class="rule">
  </header>

  <main class="doc">
{body}{around}
  </main>

  <footer class="foot">
    <p>CSSC, by lilias hatterscheidt.</p>
    <p>Write to me at
      <span class="mail" data-user="hatterscheidt.lilias" data-host="gmail.com"
        >hatterscheidt.lilias (at) gmail (dot) com</span>, or find me on
      <a href="https://github.com/liliassg">github</a>.</p>
  </footer>

</div>

<script src="../cssc.js"></script>
</body>
</html>
"""


def render(text):
    body = markdown.markdown(
        text,
        extensions=["tables", "fenced_code", "attr_list"],
        output_format="html5",
    )

    # Every fenced block in these chapters is CSSC, so hand them all to the
    # highlighter in cssc.js. It reads pre code.cssc and colors it in place.
    body = body.replace("<pre><code>", '<pre><code class="cssc">')
    body = re.sub(r'<pre><code class="language-[^"]*">',
                  '<pre><code class="cssc">', body)

    # Links between chapters point at .md files, which is right on GitHub and
    # wrong here.
    body = re.sub(r'href="([^"]+)\.md"', r'href="\1.html"', body)
    body = body.replace('href="README.html"', 'href="index.html"')

    # Tables need the horizontal scroll wrapper the rest of the site uses.
    body = body.replace("<table>", '<div class="scroll"><table>')
    body = body.replace("</table>", "</table></div>")

    return "\n".join("    " + line for line in body.splitlines())


def first_heading(text, fallback):
    m = TITLE_RE.search(text)
    return m.group(1).strip() if m else fallback


def first_paragraph(text):
    """A one-line description for the tab and the share card."""
    body = TITLE_RE.sub("", text, count=1)
    for block in body.split("\n\n"):
        block = block.strip()
        if not block or block.startswith(("#", "|", "```", ">", "-", "*")):
            continue
        line = " ".join(block.split())
        line = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", line)
        line = line.replace("`", "")
        return line[:180]
    return "The CSSC language reference."


def main():
    OUT.mkdir(parents=True, exist_ok=True)

    chapters = sorted(p for p in DOCS.glob("*.md") if p.name != "README.md")
    order = [(p, p.stem + ".html") for p in chapters]

    readme = DOCS / "README.md"
    text = readme.read_text(encoding="utf-8")
    (OUT / "index.html").write_text(
        page(
            "CSSC Language Reference",
            first_paragraph(text),
            render(text),
            None,
            (order[0][1], first_heading(chapters[0].read_text(encoding="utf-8"), "next")),
        ),
        encoding="utf-8",
    )
    print(f"  index.html          <- {readme.name}")

    for i, (path, out_name) in enumerate(order):
        text = path.read_text(encoding="utf-8")
        title = first_heading(text, path.stem)

        prev_link = None
        if i == 0:
            prev_link = ("index.html", "the chapters")
        else:
            p = order[i - 1]
            prev_link = (p[1], first_heading(p[0].read_text(encoding="utf-8"), "back"))

        next_link = None
        if i + 1 < len(order):
            n = order[i + 1]
            next_link = (n[1], first_heading(n[0].read_text(encoding="utf-8"), "next"))

        (OUT / out_name).write_text(
            page(f"{title} - CSSC", first_paragraph(text), render(text),
                 prev_link, next_link),
            encoding="utf-8",
        )
        print(f"  {out_name:22}<- {path.name}")

    print(f"\n{len(order) + 1} pages in {OUT}")


if __name__ == "__main__":
    main()
