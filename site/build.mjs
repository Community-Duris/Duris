import { readFile, writeFile, mkdir, rm, cp } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { execFileSync } from "node:child_process";
import MarkdownIt from "markdown-it";
import GithubSlugger from "github-slugger";
import hljs from "highlight.js";
import { build } from "esbuild";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "..");
const repository = process.env.GITHUB_REPOSITORY || "Community-Duris/Duris";
const base = process.env.SITE_BASE_PATH || `/${repository.split("/")[1]}/`;
if (!/^\/(?:[a-zA-Z0-9._-]+\/)*$/.test(base))
  throw new Error("Invalid SITE_BASE_PATH");
const origin =
  process.env.SITE_ORIGIN ||
  `https://${repository.split("/")[0].toLowerCase()}.github.io`;
const out = path.join(root, "bin/pages", base);
const revision =
  process.env.GITHUB_SHA ||
  execFileSync("git", ["rev-parse", "HEAD"], {
    cwd: root,
    encoding: "utf8",
  }).trim();
const branch = "master";
const github = `https://github.com/${repository}`;
const catalog = JSON.parse(
  await readFile(path.join(here, "catalog.json"), "utf8"),
);
const tracked = new Set(
  execFileSync("git", ["ls-files", "-z"], {
    cwd: root,
    encoding: "utf8",
  }).split("\0"),
);
const groups = [
  "Getting started",
  "Engineering",
  "Operations",
  "World building",
];
const routes = new Map(
  catalog.map((doc) => [doc.source, `${base}docs/${doc.slug}/`]),
);
const images = new Set();
const esc = (value) =>
  String(value).replace(
    /[&<>"']/g,
    (c) =>
      ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[
        c
      ],
  );
const encoded = (value) => value.split("/").map(encodeURIComponent).join("/");
const arrow =
  '<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" aria-hidden="true"><path d="M4 12h15m-6-6 6 6-6 6"/></svg>';
const external =
  '<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" aria-hidden="true"><path d="M6 18 18 6M6 6h12v12"/></svg>';
const search =
  '<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" aria-hidden="true"><circle cx="10.5" cy="10.5" r="6.5"/><path d="m16 16 5 5"/></svg>';

function rewriteUrl(url, source, image = false) {
  if (/^(?:[a-z][a-z\d+.-]*:|\/\/|#)/i.test(url)) return url;
  const [, pathname, suffix = ""] = url.match(/^([^?#]*)(.*)$/);
  const target = path.posix.normalize(
    path.posix.join(path.posix.dirname(source), decodeURIComponent(pathname)),
  );
  if (routes.has(target) && !image) return `${routes.get(target)}${suffix}`;
  if (image && target.startsWith("docs/assets/") && tracked.has(target)) {
    images.add(target);
    return `${base}source-assets/${encoded(target)}${suffix}`;
  }
  return `${github}/${image ? "raw" : "blob"}/${revision}/${encoded(target)}${suffix}`;
}

const md = new MarkdownIt({ html: false, linkify: true, typographer: false });
md.renderer.rules.link_open = (tokens, idx, options, env, renderer) => {
  tokens[idx].attrSet(
    "href",
    rewriteUrl(tokens[idx].attrGet("href"), env.source),
  );
  return renderer.renderToken(tokens, idx, options);
};
const defaultImage = md.renderer.rules.image;
md.renderer.rules.image = (tokens, idx, options, env, renderer) => {
  tokens[idx].attrSet(
    "src",
    rewriteUrl(tokens[idx].attrGet("src"), env.source, true),
  );
  tokens[idx].attrSet("loading", "lazy");
  return defaultImage(tokens, idx, options, env, renderer);
};
md.renderer.rules.fence = (tokens, idx) => {
  const token = tokens[idx];
  const lang = token.info.trim().split(/\s+/)[0];
  if (lang === "mermaid")
    return `<figure class="diagram"><pre class="mermaid">${esc(token.content)}</pre><figcaption>Diagram from the source document.</figcaption></figure>`;
  const code =
    lang && hljs.getLanguage(lang)
      ? hljs.highlight(token.content, { language: lang, ignoreIllegals: true })
          .value
      : esc(token.content);
  return `<div class="code-block"><div class="code-toolbar"><span>${esc(lang || "text")}</span><button class="copy-code" type="button" hidden aria-label="Copy code">Copy</button></div><pre><code>${code}</code></pre></div>`;
};
const renderTableOpen = md.renderer.rules.table_open;
md.renderer.rules.table_open = (...args) =>
  `<div class="table-scroll" role="region" aria-label="Scrollable table" tabindex="0">${renderTableOpen ? renderTableOpen(...args) : "<table>"}`;
md.renderer.rules.table_close = () => "</table></div>";

// Publish the tracked standalone diagrams without changing their artwork or styles.
const diagrams = await Promise.all(
  [...tracked]
    .filter((source) => /^docs\/diagrams\/.+\.html$/.test(source))
    .sort()
    .map(async (source) => {
      const html = await readFile(path.join(root, source), "utf8");
      const title = html.match(/<h1\b[^>]*>([\s\S]*?)<\/h1>/i)?.[1];
      const description = html.match(/<desc\b[^>]*>([\s\S]*?)<\/desc>/i)?.[1];
      if (!title || !description)
        throw new Error(
          `Diagram needs a heading and accessible description: ${source}`,
        );
      const text = (value) =>
        md.utils.unescapeAll(value.replace(/<[^>]+>/g, "")).trim();
      const url = `${base}${encoded(source.slice("docs/".length))}`;
      routes.set(source, url);
      return {
        source,
        url,
        title: text(title),
        description: text(description),
      };
    }),
);

function renderDoc(text, source) {
  const env = { source };
  const tokens = md.parse(text, env);
  const slugger = new GithubSlugger();
  const toc = [];
  let titleId = "";
  for (let i = 0; i < tokens.length; i++) {
    if (tokens[i].type !== "heading_open") continue;
    const inline = tokens[i + 1];
    const title = inline.children
      .filter((t) => ["text", "code_inline", "image"].includes(t.type))
      .map((t) => t.content)
      .join("");
    const id = slugger.slug(title);
    tokens[i].attrSet("id", id);
    if (i === 0 && tokens[i].tag === "h1") {
      titleId = id;
      tokens.splice(i, 3);
      i--;
    } else if (tokens[i].tag === "h2") {
      toc.push({ id, title });
    }
  }
  return { html: md.renderer.render(tokens, md.options, env), toc, titleId };
}

function header() {
  return `<a class="skip-link" href="#main">Skip to content</a><header class="site-header"><a class="wordmark" href="${base}" aria-label="DurisMUD home">DURIS<span>MUD</span></a><nav aria-label="Main navigation"><a href="${base}#explore">Explore</a><a href="${base}#documentation">Documentation</a><a href="${github}">GitHub ${external}</a></nav></header>`;
}
function footer() {
  return `<footer class="site-footer"><a href="${base}">DurisMUD</a><a href="${github}/tree/${revision}">Built from the repository. ${external}</a></footer>`;
}
function layout(title, description, route, content, reader = false) {
  return `<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><meta name="color-scheme" content="dark"><title>${esc(title)} · DurisMUD</title><meta name="description" content="${esc(description)}"><meta name="theme-color" content="#101b19"><link rel="canonical" href="${origin}${route}"><meta property="og:title" content="${esc(title)} · DurisMUD"><meta property="og:description" content="${esc(description)}"><meta property="og:type" content="website"><meta property="og:url" content="${origin}${route}"><meta property="og:image" content="${origin}${base}assets/citadel.webp"><link rel="icon" href="${base}assets/favicon.svg" type="image/svg+xml"><link rel="preload" href="${base}assets/fonts/cormorant-garamond-latin-400-normal.woff2" as="font" type="font/woff2" crossorigin><link rel="preload" href="${base}assets/fonts/inter-latin-400-normal.woff2" as="font" type="font/woff2" crossorigin><link rel="stylesheet" href="${base}assets/site.css"><script type="module" src="${base}assets/app.js"></script></head><body data-base="${base}" class="${reader ? "reader" : "home"}">${header()}${content}${footer()}</body></html>`;
}
function guideRow(doc) {
  return `<a class="guide-row" data-slug="${doc.slug}" data-group="${doc.group}" href="${routes.get(doc.source)}"><span class="guide-group">${doc.group}</span><h3>${esc(doc.title)} ${arrow}</h3><p>${esc(doc.description)}</p></a>`;
}
function sidebar(current) {
  return `<aside class="docs-sidebar"><details class="docs-menu" open><summary>Browse documentation</summary><div class="docs-menu-content"><a class="sidebar-title" href="${base}#documentation">Documentation</a><a class="sidebar-search" href="${base}?search=1#documentation">${search} Search all guides</a><nav aria-label="Documentation">${groups
    .map(
      (group) =>
        `<div class="sidebar-group"><p>${group}</p>${catalog
          .filter((d) => d.group === group)
          .map(
            (d) =>
              `<a href="${routes.get(d.source)}"${d.slug === current ? ' aria-current="page"' : ""}>${esc(d.title)}</a>`,
          )
          .join("")}</div>`,
    )
    .join("")}</nav></div></details></aside>`;
}

await rm(out, { recursive: true, force: true });
await mkdir(path.join(out, "assets/fonts"), { recursive: true });
await cp(path.join(here, "assets"), path.join(out, "assets"), {
  recursive: true,
});
await cp(path.join(here, "styles.css"), path.join(out, "assets/site.css"));
for (const [font, weights] of [
  ["cormorant-garamond", [400]],
  ["inter", [400, 500, 600]],
]) {
  await cp(
    path.join(here, "node_modules/@fontsource", font, "LICENSE"),
    path.join(out, "assets/fonts", `${font}-LICENSE.txt`),
  );
  for (const weight of weights) {
    const file = `${font}-latin-${weight}-normal.woff2`;
    await cp(
      path.join(here, "node_modules/@fontsource", font, "files", file),
      path.join(out, "assets/fonts", file),
    );
  }
}
await build({
  entryPoints: [path.join(here, "app.js")],
  outdir: path.join(out, "assets"),
  bundle: true,
  minify: true,
  format: "esm",
  splitting: true,
  target: ["es2022"],
  logLevel: "warning",
});

const index = `<main id="main"><section class="hero"><img class="hero-art" src="${base}assets/citadel.webp" alt="A warm-lit stone citadel above a misty lake and an ancient arched bridge" width="1942" height="809" fetchpriority="high"><div class="hero-copy"><h1>A world worth<br>understanding.</h1><p>Explore the code, systems, and craft behind DurisMUD.</p><a class="button" href="#documentation">Explore documentation ${external}</a></div></section><section class="explore" id="explore" aria-labelledby="explore-title"><h2 id="explore-title">Explore the project</h2><div class="categories"><a href="#documentation"><span class="category-number">01 /</span><div><h3>Documentation</h3><p>Guides to the world behind the game.</p></div>${arrow}</a><a href="${base}diagrams/"><span class="category-number">02 /</span><div><h3>Diagrams</h3><p>See how the systems fit together.</p></div>${arrow}</a><a href="${github}"><span class="category-number">03 /</span><div><h3>Source code</h3><p>The engine, tools, and world data.</p></div>${arrow}</a><a href="${github}/pulls"><span class="category-number">04 /</span><div><h3>Development</h3><p>Follow changes and contribute.</p></div>${arrow}</a></div></section><section class="library" id="documentation" aria-labelledby="library-title"><div class="library-heading"><div><h2 id="library-title">The documentation library</h2><p>Find your way in. Then go deeper.</p></div><form class="search-form" role="search" hidden><label class="search-field">${search}<span class="sr-only">Search documentation</span><input type="search" name="q" id="search" placeholder="Search documentation" autocomplete="off"></label></form></div><div class="filters" role="group" aria-label="Filter guides" hidden>${["All guides", ...groups].map((group, i) => `<button type="button" data-filter="${group}" aria-pressed="${i === 0}">${group}</button>`).join("")}</div><p class="search-status sr-only" role="status" aria-live="polite"></p><div class="guide-list">${catalog.map(guideRow).join("")}</div><div class="empty-state" hidden><h3>No guides found</h3><p>Try a different search or browse all guides.</p><button type="button" class="button" id="clear-search">Clear search & filters</button></div><a class="complete-index" href="${routes.get("docs/README_docs.md")}">Open the complete repository index ${arrow}</a></section></main>`;
await writeFile(
  path.join(out, "index.html"),
  layout(
    "Explore the project",
    "Explore the code, systems, and craft behind DurisMUD. Browse guides for developers, operators, and world builders.",
    base,
    index,
  ),
);

const diagramsRoute = `${base}diagrams/`;
await mkdir(path.join(out, "diagrams"), { recursive: true });
for (const diagram of diagrams) {
  const destination = path.join(out, diagram.source.slice("docs/".length));
  await mkdir(path.dirname(destination), { recursive: true });
  await cp(path.join(root, diagram.source), destination);
}
const gallery = `<main id="main" class="diagram-gallery"><div class="breadcrumb"><a href="${base}#explore">Explore the project</a><span>/</span><span>Diagrams</span></div><header class="doc-heading"><h1>Diagrams</h1><p>A closer look at the systems behind the world.</p></header><nav class="diagram-jump" aria-label="Choose a diagram">${diagrams.map((diagram, i) => `<a href="#diagram-${i + 1}">${esc(diagram.title)} ${arrow}</a>`).join("")}</nav>${diagrams.map((diagram, i) => `<section class="diagram-entry" aria-labelledby="diagram-${i + 1}"><h2 id="diagram-${i + 1}">${esc(diagram.title)}</h2><p>${esc(diagram.description)}</p><div class="source-links"><a href="${diagram.url}" target="_blank" rel="noopener">Open full size ${external}<span class="sr-only"> (opens in a new tab)</span></a><a href="${github}/blob/${revision}/${encoded(diagram.source)}">View source ${external}</a></div><iframe class="diagram-frame" src="${diagram.url}" title="${esc(diagram.title)} diagram" loading="lazy" sandbox="allow-same-origin"></iframe><p class="diagram-hint">Scroll sideways to explore larger diagrams.</p></section>`).join("")}<a class="complete-index" href="${base}#explore">${arrow} Back to the project</a></main>`;
await writeFile(
  path.join(out, "diagrams/index.html"),
  layout(
    "Diagrams",
    "Explore the DurisMUD server architecture and database model directly from the repository diagrams.",
    diagramsRoute,
    gallery,
  ),
);

const searchIndex = [];
const slugs = new Set();
for (const doc of catalog) {
  if (!/^[a-z0-9-]+$/.test(doc.slug) || slugs.has(doc.slug))
    throw new Error(`Invalid or duplicate slug: ${doc.slug}`);
  slugs.add(doc.slug);
  if (doc.source !== "README.md" && !/^docs\/[\w/-]+\.md$/.test(doc.source))
    throw new Error(`Invalid document source: ${doc.source}`);
  if (!groups.includes(doc.group))
    throw new Error(`Unknown group: ${doc.group}`);
  const text = await readFile(path.join(root, doc.source), "utf8");
  const rendered = renderDoc(text, doc.source);
  const content = `<div class="docs-layout">${sidebar(doc.slug)}<main id="main" class="doc-main"><div class="breadcrumb"><a href="${base}#documentation">Documentation</a><span>/</span><a href="${base}?category=${encodeURIComponent(doc.group)}#documentation">${doc.group}</a></div><header class="doc-heading"><h1 id="${esc(rendered.titleId || doc.slug)}">${esc(doc.title)}</h1><p>${esc(doc.description)}</p><div class="source-links"><a href="${github}/blob/${revision}/${encoded(doc.source)}">View source ${external}</a><a href="${github}/edit/${branch}/${encoded(doc.source)}">Edit on GitHub ${external}</a></div></header><article class="prose">${rendered.html}</article><div class="doc-end"><a href="${base}#documentation">${arrow} Back to all guides</a><a href="#main">Back to top</a></div></main><aside class="toc"><nav aria-label="On this page"><p>On this page</p>${rendered.toc.map((h) => `<a href="#${esc(h.id)}">${esc(h.title)}</a>`).join("")}</nav></aside></div>`;
  const destination = path.join(out, "docs", doc.slug);
  await mkdir(destination, { recursive: true });
  await writeFile(
    path.join(destination, "index.html"),
    layout(doc.title, doc.description, routes.get(doc.source), content, true),
  );
  searchIndex.push({ ...doc, url: routes.get(doc.source), text });
}
for (const source of images) {
  const target = path.join(out, "source-assets", source);
  await mkdir(path.dirname(target), { recursive: true });
  await cp(path.join(root, source), target);
}
await writeFile(
  path.join(out, "search-index.json"),
  JSON.stringify(searchIndex),
);
await writeFile(
  path.join(out, "build-info.json"),
  JSON.stringify({
    repository,
    revision,
    base,
    guides: catalog.length,
    diagrams: diagrams.length,
  }),
);
await writeFile(path.join(out, ".nojekyll"), "");
await writeFile(
  path.join(out, "sitemap.xml"),
  `<?xml version="1.0" encoding="UTF-8"?><urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">${[base, diagramsRoute, ...routes.values()].map((url) => `<url><loc>${origin}${url}</loc></url>`).join("")}</urlset>`,
);
await writeFile(
  path.join(out, "404.html"),
  layout(
    "Page not found",
    "Find your way back to the DurisMUD documentation library.",
    `${base}404.html`,
    `<main id="main" class="not-found"><h1>This path is uncharted.</h1><p>The page may have moved. Start again from the guide library.</p><a class="button" href="${base}#documentation">Explore documentation ${arrow}</a></main>`,
  ),
);
console.log(
  `Built ${catalog.length} guides and ${diagrams.length} diagrams from the repository in ${path.relative(root, out)}`,
);
