# Project website

The [DurisMUD project hub](https://community-duris.github.io/Duris/) is published
from this repository with GitHub Pages and GitHub Actions. It provides a project
index, a searchable documentation library, and a reader for selected guides.

## Content comes from the repository

`site/catalog.json` selects the source Markdown, title, summary, category, and
stable URL for each published guide. The build renders those source files directly;
there is no second copy of the documentation to maintain. The complete repository
index remains available for references outside the selected library.

To publish another guide, add an entry to the catalog with a unique lowercase,
hyphenated slug and one of the existing categories. Edit the original Markdown
to update a guide. Headings become linkable anchors, fenced code gets syntax
highlighting, and Mermaid blocks become diagrams when they enter the viewport.
The reader retains diagram source when JavaScript is disabled or rendering fails.

Links between published guides stay on the website. Other repository links and
the **View source** action open the exact Git revision used to build the site.
**Edit on GitHub** opens the original file on `master`. Full-text search runs in
the browser against an index produced from the same Markdown files; search and
category choices can be shared through the URL.

## Diagrams

The [Diagrams gallery](https://community-duris.github.io/Duris/diagrams/) displays
the standalone HTML diagrams tracked under `docs/diagrams/`. The build discovers
these files automatically, reads each diagram's heading and SVG description for
the gallery, and publishes the original HTML unchanged. Each diagram has an
embedded view, a full-size view, and a link to its source revision on GitHub.
Documentation links to these files open the published diagrams.

To add a diagram, commit its standalone `.html` file under `docs/diagrams/` with
an `<h1>` title and an SVG `<desc>` description. Changes publish through the same
GitHub Actions workflow. Embedded views isolate the original styles and disable
scripts; their full-size pages retain the original document behavior.

## Power Atlas

The [Power Atlas](https://community-duris.github.io/Duris/power-atlas/) publishes
the supplied combat-model report as an interactive page. `site/power-atlas/`
contains its report content, styles, interaction code, and unchanged JSON snapshot.
The snapshot covers 192 race/class combinations, 711 single-class builds, 56
Human and Orc multiclass builds, 13 report levels, and three gear tiers. Its
model source revision is `f3b66b07f`; publishing the website does not regenerate
the model or repeat its reported engine checks.

Level, gear, metric, and specialisation settings are stored in the page URL.
**Copy atlas view link** shares those settings. Original `#L=…&tier=…` links
remain supported. Tables scroll within the page; clicking, tapping, or pressing
Enter on an atlas cell opens its full breakdown. Escape closes the details.
The findings and methodology are readable without JavaScript, and the complete
snapshot can be downloaded. Only the atlas page fetches the model data.

The artifact tests pin the imported snapshot's SHA-256 and verify its dimensions,
route, source revision, and local assets. A future model refresh must update the
snapshot, report, source revision, and matching tests together. Do not edit model
values while changing presentation. Browser checks should cover all report
levels, gear selection, metric, specialisation and multiclass views, shared URLs,
cell details, clipboard behavior, mobile scrolling, and a failed snapshot download.

## Build and verify locally

Use Node.js 24 or later and Python 3:

```bash
npm ci --prefix site
npm test --prefix site
python3 -m http.server 4173 --bind 127.0.0.1 --directory bin/pages
```

Open `http://127.0.0.1:4173/Duris/`. Rebuild with `npm run build --prefix site`
after editing a source document or the site. Output goes under ignored
`bin/pages/Duris/`; dependencies stay under ignored `site/node_modules/`.

The focused tests check all generated pages, local links and fragments, heading
IDs, source and edit links, source-to-search-index equality, static rich content,
and the published artifact boundary. Browser checks should also cover desktop and
mobile layout, filtering, full-text search, empty results, reader navigation,
copying code, and Mermaid diagrams. Documentation remains readable without
JavaScript; search, filters, copying, and diagram rendering progressively enhance it.

## GitHub publishing

`.github/workflows/pages.yml` builds and tests on pull requests and on pushes to
`master` that change documentation, the README, site files, or the publishing
workflow. A successful build on `master` deploys through the protected
`github-pages` environment. The workflow can also be run manually.

GitHub Pages must use **GitHub Actions** as its publishing source. The build job
has read-only repository access. Only the deployment job receives `pages: write`
and `id-token: write`. Pull requests build and test without deploying. Action
versions are pinned to commits, and npm dependencies are locked and checked by
Dependabot. The uploaded artifact contains only the generated site.

There are no runtime API credentials or GitHub API requests. The project hub's
fonts, artwork, scripts, styles, and Mermaid code are hosted with the site. The
initial page loads only its small interaction script; Mermaid dependencies load
on demand. Standalone HTML diagrams retain their original Google Fonts stylesheet
and system-font fallbacks.

The builder derives the project base path and Pages origin from
`GITHUB_REPOSITORY`, defaulting to `Community-Duris/Duris`. `SITE_BASE_PATH`
(leading and trailing slash) and `SITE_ORIGIN` can override these for another
Pages location. `GITHUB_SHA` identifies the deployed source revision.

## Visual design

The homepage uses a pine-black background (`#101b19`), ivory type (`#f2eee3`),
sage secondary text (`#aabbb0`), and muted gold accents (`#c6a369`). Cormorant
Garamond provides editorial headings and Inter provides interface and reading
text. Their font files are bundled from the Fontsource packages.

`site/assets/citadel.webp` is the generated production artwork. The homepage and
reader were designed with the built-in image generation tool, and the standalone
hero was generated from the homepage concept with this prompt:

> The reference is a design concept for the DurisMUD project homepage. Extract its
> fantasy landscape as a standalone production web hero image: the same magnificent
> aged stone citadel on a rocky island to the right, warm windows, arched stone
> bridge, dark mountains, misty lake and pine forests, same painterly realistic
> texture and fine detail. Wide 1920x800 composition. Preserve the distinctive
> castle and muted dark pine/gray and warm stone palette. Leftmost 40 percent fades
> naturally to a nearly uniform pine-black #101b19 for HTML headline placement;
> top and bottom edges blend naturally into #101b19. Keep main citadel in right half
> and entire castle silhouette visible. Remove ALL text, logo, UI, rules, footer,
> search, buttons, and categories. Only a beautiful fantasy landscape backdrop.
> No text or watermark.

Keep artwork separate from accessible HTML text and controls. The mobile layout
stacks categories and guide rows and collapses the documentation navigation into
a native disclosure. Honor reduced-motion preferences when changing transitions.
