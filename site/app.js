const base = document.body.dataset.base;
const searchInput = document.querySelector("#search");

const docsMenu = document.querySelector(".docs-menu");
if (docsMenu) {
  const mobile = window.matchMedia("(max-width: 800px)");
  const syncMenu = () => {
    docsMenu.open = !mobile.matches;
  };
  syncMenu();
  mobile.addEventListener("change", syncMenu);
}

if (searchInput) {
  const form = document.querySelector(".search-form");
  const filters = [...document.querySelectorAll("[data-filter]")];
  const rows = [...document.querySelectorAll(".guide-row")];
  const status = document.querySelector(".search-status");
  const empty = document.querySelector(".empty-state");
  const params = new URLSearchParams(location.search);
  let category = params.get("category") || "All guides";
  if (!filters.some((button) => button.dataset.filter === category))
    category = "All guides";
  let index;
  let indexRequest;
  let updateId = 0;
  form.hidden = false;
  document.querySelector(".filters").hidden = false;
  searchInput.value = params.get("q") || "";

  async function loadIndex() {
    if (index) return index;
    if (!indexRequest) {
      indexRequest = fetch(`${base}search-index.json`)
        .then((response) => {
          if (!response.ok) throw new Error("Search index unavailable");
          return response.json();
        })
        .then((docs) => {
          index = new Map(
            docs.map((doc) => [
              doc.slug,
              `${doc.title} ${doc.description} ${doc.text}`.toLowerCase(),
            ]),
          );
          return index;
        })
        .catch(() => {
          indexRequest = undefined;
          return undefined;
        });
    }
    return indexRequest;
  }

  async function update() {
    const currentUpdate = ++updateId;
    const query = searchInput.value.trim().toLowerCase();
    const words = query.split(/\s+/).filter(Boolean);
    filters.forEach((button) =>
      button.setAttribute(
        "aria-pressed",
        String(button.dataset.filter === category),
      ),
    );
    const fullText = query ? await loadIndex() : index;
    if (currentUpdate !== updateId) return;
    let count = 0;
    for (const row of rows) {
      const text =
        fullText?.get(row.dataset.slug) || row.textContent.toLowerCase();
      row.hidden =
        (category !== "All guides" && row.dataset.group !== category) ||
        !words.every((word) => text.includes(word));
      if (!row.hidden) count++;
    }
    empty.hidden = count !== 0;
    status.classList.toggle("sr-only", Boolean(fullText) || !query);
    status.textContent = `${count} ${count === 1 ? "guide" : "guides"} found.${query && !fullText ? " Full-text search is unavailable; searching titles and summaries." : ""}`;
    const next = new URLSearchParams();
    if (searchInput.value.trim()) next.set("q", searchInput.value.trim());
    if (category !== "All guides") next.set("category", category);
    history.replaceState(
      null,
      "",
      `${location.pathname}${next.size ? `?${next}` : ""}${location.hash}`,
    );
  }

  form.addEventListener("submit", (event) => {
    event.preventDefault();
    update();
  });
  searchInput.addEventListener("input", update);
  searchInput.addEventListener("focus", loadIndex, { once: true });
  filters.forEach((button) =>
    button.addEventListener("click", () => {
      category = button.dataset.filter;
      update();
    }),
  );
  document.querySelector("#clear-search").addEventListener("click", () => {
    category = "All guides";
    searchInput.value = "";
    update();
    searchInput.focus();
  });
  update();
  if (params.has("search")) searchInput.focus();
}

for (const button of document.querySelectorAll(".copy-code")) {
  if (!navigator.clipboard?.writeText) continue;
  button.hidden = false;
  button.addEventListener("click", async () => {
    const code = button
      .closest(".code-block")
      .querySelector("code").textContent;
    try {
      await navigator.clipboard.writeText(code);
      button.textContent = "Copied";
      button.setAttribute("aria-label", "Code copied");
    } catch {
      button.textContent = "Select code";
      button.setAttribute(
        "aria-label",
        "Copy unavailable; select code manually",
      );
      const range = document.createRange();
      range.selectNodeContents(
        button.closest(".code-block").querySelector("code"),
      );
      const selection = window.getSelection();
      selection.removeAllRanges();
      selection.addRange(range);
    }
    setTimeout(() => {
      button.textContent = "Copy";
      button.setAttribute("aria-label", "Copy code");
    }, 2200);
  });
}

const tocLinks = [...document.querySelectorAll(".toc a")];
if (tocLinks.length) {
  const observer = new IntersectionObserver(
    (entries) => {
      for (const entry of entries) {
        if (!entry.isIntersecting) continue;
        tocLinks.forEach((link) => {
          if (decodeURIComponent(link.hash.slice(1)) === entry.target.id)
            link.setAttribute("aria-current", "true");
          else link.removeAttribute("aria-current");
        });
      }
    },
    { rootMargin: "-100px 0px -60% 0px" },
  );
  document
    .querySelectorAll(".prose h2")
    .forEach((heading) => observer.observe(heading));
}

const diagrams = [...document.querySelectorAll(".mermaid")];
if (diagrams.length) {
  let mermaidRequest;
  const observer = new IntersectionObserver(
    (entries) => {
      entries
        .filter((entry) => entry.isIntersecting)
        .forEach(async ({ target }) => {
          observer.unobserve(target);
          const original = target.textContent;
          try {
            mermaidRequest ||= import("mermaid").then(
              ({ default: mermaid }) => {
                mermaid.initialize({
                  startOnLoad: false,
                  securityLevel: "strict",
                  theme: "dark",
                  layout: "dagre",
                  fontFamily: "Inter, sans-serif",
                  themeVariables: {
                    primaryColor: "#243a2e",
                    primaryTextColor: "#f2eee3",
                    primaryBorderColor: "#708271",
                    lineColor: "#c6a369",
                    background: "#101b19",
                  },
                });
                return mermaid;
              },
            );
            const mermaid = await mermaidRequest;
            await mermaid.run({ nodes: [target] });
          } catch {
            target.textContent = original;
            target.closest("figure").querySelector("figcaption").textContent =
              "Diagram source. The visual preview could not be loaded.";
          }
        });
    },
    { rootMargin: "300px" },
  );
  diagrams.forEach((diagram) => observer.observe(diagram));
}
