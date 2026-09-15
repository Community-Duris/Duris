"""Check the generated Pages artifact, including every local link and anchor."""

import json
import os
import re
import unittest
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = os.environ.get("GITHUB_REPOSITORY", "Community-Duris/Duris")
BASE = os.environ.get("SITE_BASE_PATH", f"/{REPOSITORY.split('/')[1]}/")
OUTPUT = ROOT / "bin" / "pages" / BASE.strip("/")


class Page(HTMLParser):
    def __init__(self, content):
        super().__init__(convert_charrefs=True)
        self.links = []
        self.ids = []
        self.h1_count = 0
        self.text = []
        self.feed(content)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if "id" in attrs:
            self.ids.append(attrs["id"])
        if tag == "h1":
            self.h1_count += 1
        for attribute in ("href", "src"):
            if attribute in attrs:
                self.links.append(attrs[attribute])

    def handle_data(self, data):
        self.text.append(data)


class PagesTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.catalog = json.loads((ROOT / "site/catalog.json").read_text())
        cls.pages = {
            file: Page(file.read_text()) for file in OUTPUT.rglob("*.html")
        }

    def test_all_curated_guides_are_published(self):
        self.assertEqual(len(self.pages), len(self.catalog) + 2)
        for doc in self.catalog:
            page = OUTPUT / "docs" / doc["slug"] / "index.html"
            self.assertIn(page, self.pages)
            self.assertIn(doc["title"], "".join(self.pages[page].text))

    def test_local_links_assets_and_fragments_resolve(self):
        failures = []
        for file, page in self.pages.items():
            for link in page.links:
                url = urlsplit(link)
                if url.scheme or url.netloc:
                    continue
                if url.path.startswith("/"):
                    if not url.path.startswith(BASE):
                        failures.append(f"{file}: link escapes Pages base: {link}")
                        continue
                    target = OUTPUT / unquote(url.path[len(BASE):])
                else:
                    target = file.parent / unquote(url.path) if url.path else file
                if target.is_dir():
                    target = target / "index.html"
                if not target.is_file():
                    failures.append(f"{file}: missing target: {link}")
                elif url.fragment and target in self.pages:
                    if unquote(url.fragment) not in self.pages[target].ids:
                        failures.append(f"{file.relative_to(OUTPUT)}: missing anchor: {link}")
        self.assertEqual(failures, [])

    def test_heading_ids_are_unique_and_one_primary_heading(self):
        for file, page in self.pages.items():
            with self.subTest(page=file.relative_to(OUTPUT)):
                self.assertEqual(page.h1_count, 1)
                self.assertEqual(len(page.ids), len(set(page.ids)))

    def test_search_uses_current_repository_content(self):
        index = json.loads((OUTPUT / "search-index.json").read_text())
        self.assertEqual(len(index), len(self.catalog))
        for doc in index:
            self.assertEqual(doc["text"], (ROOT / doc["source"]).read_text())
            self.assertEqual(doc["url"], f"{BASE}docs/{doc['slug']}/")

    def test_documents_include_source_and_edit_links(self):
        metadata = json.loads((OUTPUT / "build-info.json").read_text())
        for doc in self.catalog:
            page = self.pages[OUTPUT / "docs" / doc["slug"] / "index.html"]
            self.assertTrue(any(f"/blob/{metadata['revision']}/{doc['source']}" in link for link in page.links))
            self.assertTrue(any(f"/edit/master/{doc['source']}" in link for link in page.links))

    def test_static_reader_contains_tables_code_and_diagram_source(self):
        html = (OUTPUT / "docs/quick-start/index.html").read_text()
        self.assertIn('<pre class="mermaid">', html)
        self.assertIn('flowchart LR', html)
        self.assertIn('<table>', html)
        self.assertIn('make test-all', ''.join(self.pages[OUTPUT / 'docs/quick-start/index.html'].text))
        self.assertIn('class="code-block"', html)

    def test_artifact_contains_only_site_outputs(self):
        for file in OUTPUT.rglob("*"):
            self.assertFalse(file.is_symlink(), file)
            self.assertNotIn(file.name, {".env", ".env.docker", "AGENTS.md", "package-lock.json"})
            self.assertFalse(re.search(r"\.(?:sql|key|log|pem)$", file.name), file)


if __name__ == "__main__":
    unittest.main(verbosity=2)
