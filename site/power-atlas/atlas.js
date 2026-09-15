function initAtlas(D) {
  const LV = D.meta.levels;
  const TIERS = ["basic", "good", "endgame"];
  const TL = { basic: "Basic", good: "Good", endgame: "End-game" };
  const ht = (L) => (L <= 35 ? "basic" : L <= 50 ? "good" : "endgame");
  const RN = (r) => D.raceName[r] || r;
  const ABBR = {
    Warrior: "War",
    Mercenary: "Merc",
    Berserker: "Bers",
    Ranger: "Rang",
    Paladin: "Pal",
    "Anti-Paladin": "A-Pal",
    AntiPaladin: "A-Pal",
    Reaver: "Reav",
    Monk: "Monk",
    Rogue: "Rog",
    Bard: "Bard",
    Cleric: "Cler",
    Druid: "Dru",
    Shaman: "Sham",
    Blighter: "Blig",
    Sorcerer: "Sorc",
    Conjurer: "Conj",
    Summoner: "Summ",
    Necromancer: "Necr",
    Illusionist: "Illu",
    Ethermancer: "Ethr",
    Psionicist: "Psi",
  };
  const ab = (c) => ABBR[c] || c.slice(0, 4);
  const combo = (k) => {
    const [r, c] = k.split("|");
    return RN(r) + " " + c;
  };
  const F = {
    idx: 0,
    win: 1,
    pve: 2,
    hp: 3,
    round: 4,
    mel: 5,
    spl: 6,
    heal: 7,
    ehpm: 8,
    ehps: 9,
    shrug: 10,
    save: 11,
    burst: 12,
    spell: 13,
    bkind: 14,
    hname: 15,
  };
  const $ = (id) => document.getElementById(id);
  const esc = (s) =>
    String(s == null ? "" : s).replace(
      /[&<>"]/g,
      (ch) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" })[ch],
    );

  $("gen").textContent = D.meta.generated;
  if (D.meta.field === "full" && D.specs) {
    $("facts-n-label").textContent = "Builds from L31";
    $("facts-n").textContent = Object.keys(
      D.specs.variants,
    ).length.toLocaleString();
  }
  const nMulti = D.multi ? Object.keys(D.multi.builds || {}).length : 0;
  if (nMulti) $("facts-multi").textContent = nMulti.toLocaleString();
  else $("facts-multi").closest("div").hidden = true;
  $("foot-gen").textContent =
    "Model run " +
    D.meta.generated +
    " · " +
    D.meta.combos +
    " combinations" +
    (nMulti ? " · " + nMulti + " multiclass builds." : ".");

  /* ---------- colour ---------- */
  const isDark = () => true;
  const RAMPS = {
    light: {
      blue: ["#f0efec", "#b7d3f6", "#6da7ec", "#2a78d6", "#1c5cab", "#104281"],
      red: ["#f0efec", "#f5c8c5", "#ec918c", "#e34948", "#b3302f", "#7e2020"],
    },
    dark: {
      blue: ["#383835", "#1e3b61", "#1d58a0", "#3987e5", "#6da7ec", "#a9cdf6"],
      red: ["#383835", "#5a2b2a", "#8e3533", "#d65150", "#e66767", "#f2a3a3"],
    },
  };
  const hexRgb = (h) => [1, 3, 5].map((i) => parseInt(h.slice(i, i + 2), 16));
  const rgbHex = (a) =>
    "#" + a.map((v) => Math.round(v).toString(16).padStart(2, "0")).join("");
  function ramp(stops, t) {
    t = Math.max(0, Math.min(1, t));
    const p = t * (stops.length - 1);
    const i = Math.min(stops.length - 2, Math.floor(p));
    const a = hexRgb(stops[i]),
      b = hexRgb(stops[i + 1]),
      f = p - i;
    return rgbHex(a.map((v, j) => v + (b[j] - v) * f));
  }
  const pal = () => RAMPS[isDark() ? "dark" : "light"];
  function divColor(v, decades) {
    if (!(v > 0)) return null;
    const t = Math.log10(v) / (decades || 1.5);
    return t < 0 ? ramp(pal().red, -t) : ramp(pal().blue, t);
  }
  function winColor(v) {
    if (v == null) return null;
    const t = (v - 0.5) * 2;
    return t < 0 ? ramp(pal().red, -t) : ramp(pal().blue, t);
  }
  const seqColor = (v, max) => ramp(pal().blue, max > 0 ? v / max : 0);
  function inkOn(hex) {
    const lum = hexRgb(hex).map((c) => {
      c /= 255;
      return c <= 0.03928 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4);
    });
    return 0.2126 * lum[0] + 0.7152 * lum[1] + 0.0722 * lum[2] > 0.179
      ? "#000000"
      : "#ffffff";
  }
  const cellStyle = (bg) => (bg ? `background:${bg};color:${inkOn(bg)}` : "");

  function fIdx(v) {
    if (v == null) return "–";
    if (v >= 10) return v.toFixed(1);
    if (v >= 0.095) return v.toFixed(2);
    return v.toFixed(3);
  }
  const fNum = (v) =>
    v == null
      ? "–"
      : v >= 100
        ? Math.round(v).toLocaleString()
        : v >= 10
          ? v.toFixed(0)
          : v.toFixed(1);
  const fMult = (v) => "×" + (v >= 10 ? v.toFixed(1) : v.toFixed(2));

  function legend(el, kind, max) {
    let stops = [],
      ticks = [];
    if (kind === "div" || kind === "fac") {
      const dec = kind === "fac" ? 0.6 : 1.5;
      for (let i = 0; i <= 20; i++) {
        const t = -1 + i / 10;
        stops.push(t < 0 ? ramp(pal().red, -t) : ramp(pal().blue, t));
      }
      const vals =
        kind === "fac" ? [0.25, 0.5, 1, 2, 4] : [0.03, 0.1, 0.3, 1, 3, 10, 30];
      ticks = vals.map((v) => [
        (Math.log10(v) / dec + 1) / 2,
        v >= 1
          ? (kind === "fac" ? "×" : "") + v
          : (kind === "fac" ? "×" : "") + v,
      ]);
    } else if (kind === "win") {
      for (let i = 0; i <= 20; i++) {
        const t = -1 + i / 10;
        stops.push(t < 0 ? ramp(pal().red, -t) : ramp(pal().blue, t));
      }
      ticks = [
        [0, "0%"],
        [0.25, "25%"],
        [0.5, "50%"],
        [0.75, "75%"],
        [1, "100%"],
      ];
    } else {
      for (let i = 0; i <= 10; i++) stops.push(ramp(pal().blue, i / 10));
      ticks = [
        [0, "0"],
        [0.5, fNum(max / 2)],
        [1, fNum(max)],
      ];
    }
    el.innerHTML = `<div class="legend"><div><div class="bar" style="background:linear-gradient(90deg,${stops.join(",")})"></div><div class="ticks">${ticks.map(([p, l]) => `<span style="left:${(p * 100).toFixed(1)}%">${l}</span>`).join("")}</div></div></div>`;
  }

  /* ---------- atlas ---------- */
  const MET = {
    idx: {
      label: "Duel index",
      f: F.idx,
      kind: "div",
      fmt: fIdx,
      note: "Duel index: geometric mean over every opponent build of TTK(them → you) ÷ TTK(you → them). 1 = even.",
    },
    win: {
      label: "Share of duels won",
      f: F.win,
      kind: "win",
      fmt: (v) => (v == null ? "–" : Math.round(v * 100) + "%"),
      note: "Share of the opponent-build matchups this combination wins: 192 at report levels 1–26, 711 from level 31.",
    },
    pve: {
      label: "PvE kill rate",
      f: F.pve,
      kind: "div",
      fmt: fIdx,
      note: "Kill rate against a level-matched mob built by the engine's own mob formulas; 1 = typical.",
    },
    hp: {
      label: "Max HP",
      f: F.hp,
      kind: "seq",
      fmt: (v) => (v == null ? "–" : Math.round(v).toString()),
      note: "Maximum hit points with the kit, from the engine's HP formula.",
    },
    mel: {
      label: "Melee damage / second",
      f: F.mel,
      kind: "seq",
      fmt: fNum,
      note: "Melee damage per second against a level-matched player in the same gear tier, after avoidance, armour and the global reduction.",
    },
    spl: {
      label: "Spell damage / second",
      f: F.spl,
      kind: "seq",
      fmt: fNum,
      note: "Best single-target spell damage per second against a level-matched player in the same gear tier.",
    },
    heal: {
      label: "Healing / second",
      f: F.heal,
      kind: "seq",
      fmt: fNum,
      note: "Best heal per second of casting.",
    },
  };
  const st = {
    L: 56,
    tier: "endgame",
    follow: true,
    met: "idx",
    step: "50-51",
    fL: 56,
    hwL: 56,
    gL: 56,
    pvp: "56|endgame",
    focus: "",
  };
  const key = () => st.L + "|" + (st.follow ? ht(st.L) : st.tier);

  const metricSel = $("metric");
  metricSel.innerHTML = Object.entries(MET)
    .map(([k, m]) => `<option value="${k}">${m.label}</option>`)
    .join("");
  metricSel.addEventListener("change", () => {
    st.met = metricSel.value;
    renderAtlas();
  });
  $("follow").addEventListener("change", (e) => {
    if (st.follow) st.tier = ht(st.L);
    st.follow = e.target.checked;
    renderAtlas();
  });

  /* Specialisations: each race x class x spec variant scored on its own (spec_results.json). */
  const SP = D.specs;
  const SPECS_BY_CLASS = {};
  if (SP) {
    Object.keys(SP.variants).forEach((vk) => {
      const parts = vk.split("|");
      const list = (SPECS_BY_CLASS[parts[1]] = SPECS_BY_CLASS[parts[1]] || []);
      if (!list.includes(parts[2])) list.push(parts[2]);
    });
    Object.values(SPECS_BY_CLASS).forEach((l) => l.sort());
  }
  const prettySpec = (sp) =>
    sp === "BASE"
      ? "Unspecialised"
      : sp === "MULTI"
        ? "Multiclass"
        : String(sp)
            .toLowerCase()
            .replace(/_/g, " ")
            .replace(/\b\w/g, (ch) => ch.toUpperCase());
  /* What a build changes, in one line; the summaries for unspecialised start with the word itself. */
  const aboutText = (cl, sp) => {
    const a = ((SP && SP.about) || {})[cl + "|" + sp];
    const t = a
      ? String(a.summary)
      : sp === "BASE"
        ? "No specialisation: the base class's own skills and spells."
        : "";
    return t
      .replace(/^Unspecialised:\s*/i, "")
      .replace(/^\w/, (ch) => ch.toUpperCase());
  };
  /* The build behind a headline cell: one spec, "A|B" for an exact tie, or null. */
  const specLabel = (s, k) => {
    if (s)
      return s.includes("|")
        ? "tied: " + s.split("|").map(prettySpec).join(", ")
        : prettySpec(s);
    return D.meta.field === "full" && k && +k.split("|")[0] >= 30
      ? "Unspecialised"
      : "";
  };
  const focusSel = $("spec-focus");
  if (SP) {
    focusSel.innerHTML +=
      (D.meta.field === "full"
        ? ""
        : `<option value="__best__">Best specialisation</option>`) +
      D.classes
        .filter((c) => SPECS_BY_CLASS[c])
        .map(
          (c) => `<option value="${esc(c)}">${esc(c)} specialisations</option>`,
        )
        .join("");
    if (D.meta.field === "full")
      focusSel.options[0].textContent = "Best build (headline)";
    focusSel.addEventListener("change", () => {
      st.focus = focusSel.value;
      renderAtlas();
    });
  } else {
    focusSel.closest(".ctl").hidden = true;
  }
  /* The best-scoring specialisation of one combination at one level/tier: [spec, row] or null. */
  function bestVariant(r, cl, k) {
    let best = null;
    (SPECS_BY_CLASS[cl] || []).forEach((sp) => {
      const v = SP.variants[r + "|" + cl + "|" + sp];
      const row = v && v[k];
      if (row && row[0] != null && (!best || row[0] > best[1][0]))
        best = [sp, row];
    });
    return best;
  }
  function specLine(comboKey, k, pick) {
    if (!SP) return "";
    const [r, cl] = comboKey.split("|");
    const bv = bestVariant(r, cl, k);
    const items = (SPECS_BY_CLASS[cl] || [])
      .map((sp) => {
        const v = SP.variants[r + "|" + cl + "|" + sp];
        const row = v && v[k];
        return row
          ? `${prettySpec(sp)} ${fIdx(row[0])}${
              String(pick || "BASE")
                .split("|")
                .includes(sp)
                ? D.meta.field === "full"
                  ? " ✓ best"
                  : " ✓ pick"
                : ""
            }${bv && bv[0] === sp ? " ★ best" : ""}`
          : null;
      })
      .filter(Boolean);
    return items.length
      ? `<p style="margin:6px 0 0;color:var(--muted);font-size:11.5px">Specialisations: ${esc(items.join(" · "))}</p>`
      : "";
  }

  function chips(el, items, current, onPick, disabled) {
    const focused = el.contains(document.activeElement)
      ? document.activeElement.dataset.v
      : null;
    el.innerHTML = items
      .map(
        ([v, label]) =>
          `<button type="button" class="chip" data-v="${v}" aria-pressed="${String(v) === String(current)}" ${disabled ? "disabled" : ""}>${label}</button>`,
      )
      .join("");
    el.querySelectorAll("button").forEach((b) =>
      b.addEventListener("click", () => onPick(b.dataset.v)),
    );
    if (focused != null)
      [...el.querySelectorAll("button")]
        .find((b) => b.dataset.v === focused)
        ?.focus();
  }

  function renderAtlas() {
    syncViewUrl();
    if ($("tip")) $("tip").hidden = true;
    const k = key();
    const tier = k.split("|")[1];
    chips(
      $("lvl-chips"),
      LV.map((L) => [L, "L" + L]),
      st.L,
      (v) => {
        st.L = +v;
        renderAtlas();
      },
    );
    chips(
      $("tier-chips"),
      TIERS.map((t) => [t, TL[t]]),
      tier,
      (v) => {
        st.tier = v;
        renderAtlas();
      },
      st.follow,
    );
    const m = MET[st.met];
    if (st.focus && SP && st.focus !== "__best__") {
      renderFocus(k, tier, m);
      return;
    }
    const bestMode = st.focus === "__best__" && !!SP;
    /* In best mode each cell uses the combination's best-scoring specialisation at this level,
     * where one exists (level 31 and up); otherwise the headline row. */
    const rowFor = (r, c) => {
      const cc = D.combos[r + "|" + c];
      const head = cc && cc.d[k];
      if (!head) return [null, null, false];
      if (!bestMode) return [head, null, false];
      const bv = bestVariant(r, c, k);
      if (!bv) return [head, null, false];
      return [bv[1], bv[0], ((cc.s || {})[k] || "BASE") !== bv[0]];
    };
    let max = 0;
    D.races.forEach((r) =>
      D.classes.forEach((c) => {
        const row = rowFor(r, c)[0];
        if (row && row[m.f] > max) max = row[m.f];
      }),
    );
    const colour = (v) =>
      v == null
        ? null
        : m.kind === "div"
          ? divColor(v)
          : m.kind === "win"
            ? winColor(v)
            : seqColor(v, max);
    const rk = D.ranks[k] || { classes: [], races: [] };
    const clsIdx = Object.fromEntries(rk.classes.map((x) => [x[0], x[1]]));
    const raceF = Object.fromEntries(rk.races.map((x) => [x[0], x[2]]));
    const showSums = st.met === "idx" && !bestMode;
    let h =
      "<thead><tr><th></th>" +
      D.classes
        .map((c) => `<th scope="col" title="${esc(c)}">${esc(ab(c))}</th>`)
        .join("") +
      `<th class="gap"></th><th scope="col" title="Strength within its classes">Race ×</th></tr></thead><tbody>`;
    let lastSide = null;
    D.races.forEach((r) => {
      const side = D.side[r] || "";
      if (side !== lastSide) {
        h += `<tr class="side-row"><th colspan="${D.classes.length + 3}">${esc(side)}</th></tr>`;
        lastSide = side;
      }
      h += `<tr><th class="row" scope="row">${esc(RN(r))}</th>`;
      D.classes.forEach((c) => {
        const [row, , differs] = rowFor(r, c);
        if (!row) {
          h += `<td class="na" aria-label="${esc(RN(r) + " " + c)}: not a legal combination">·</td>`;
          return;
        }
        const v = row[m.f];
        const ring = differs ? ";box-shadow:inset 0 0 0 2px var(--ink)" : "";
        h += `<td tabindex="0" data-k="${esc(r + "|" + c)}" aria-haspopup="dialog" aria-label="${esc(RN(r) + " " + c + ": " + m.label + " " + m.fmt(v) + ". Open details")}" style="${cellStyle(colour(v))}${ring}">${m.fmt(v)}</td>`;
      });
      const rf = raceF[r];
      h += `<td class="gap"></td><td class="sum" style="${showSums && rf ? cellStyle(divColor(rf, 0.6)) : ""}">${showSums && rf ? fMult(rf) : ""}</td></tr>`;
    });
    h +=
      `<tr class="foot"><th class="row" scope="row">Class average</th>` +
      D.classes
        .map((c) => {
          const v = clsIdx[c];
          return `<td class="sum" style="${showSums && v ? cellStyle(divColor(v)) : ""}">${showSums && v ? fIdx(v) : ""}</td>`;
        })
        .join("") +
      `<td class="gap"></td><td></td></tr></tbody>`;
    $("atlas-table").innerHTML = h;
    $("atlas-caption").textContent =
      `Level ${st.L} · ${TL[tier]} gear · ${m.label}` +
      (bestMode
        ? " · best specialisation (ringed = better than the model's pick)"
        : "");
    legend($("atlas-legend"), m.kind, max);
    $("atlas-note").textContent =
      m.note +
      (bestMode
        ? " Each combination uses its best-scoring specialisation at this level and gear (specialisations open at level 30); ringed cells are where that beats the specialisation the headline uses. Summaries are hidden in this view."
        : showSums
          ? ""
          : " Race and class summaries are shown for the duel index only.");
  }

  /* atlas, one class's specialisations: races x specs */
  function renderFocus(k, tier, m) {
    const cl = st.focus;
    const L = +k.split("|")[0];
    const specs = SPECS_BY_CLASS[cl] || [];
    const openL = (SP.meta.spec_open_level || {})[cl] || 30;
    const races = D.races.filter((r) => D.combos[r + "|" + cl]);
    const cell = (r, sp) => {
      const v = SP.variants[r + "|" + cl + "|" + sp];
      return v && v[k];
    };
    let max = 0;
    races.forEach((r) =>
      specs.forEach((sp) => {
        const row = cell(r, sp);
        if (row && row[m.f] > max) max = row[m.f];
      }),
    );
    const colour = (v) =>
      v == null
        ? null
        : m.kind === "div"
          ? divColor(v)
          : m.kind === "win"
            ? winColor(v)
            : seqColor(v, max);
    const cs = Object.fromEntries(
      ((SP.class_spec[k] || {})[cl] || []).map((x) => [x[0], x]),
    );
    const hasAny = races.some((r) => specs.some((sp) => cell(r, sp)));
    let h =
      "<thead><tr><th></th>" +
      specs
        .map(
          (sp) =>
            `<th scope="col" style="min-width:96px">${esc(prettySpec(sp))}</th>`,
        )
        .join("") +
      "</tr></thead><tbody>";
    if (!hasAny) {
      h += `<tr><td colspan="${specs.length + 1}" style="text-align:left;padding:14px;font-family:var(--sans);color:var(--ink-2)">${esc(cl)} specialisations open at level ${openL}. Choose level 31 or above.</td></tr>`;
    } else {
      races.forEach((r) => {
        const picks = String(
          ((D.combos[r + "|" + cl] || {}).s || {})[k] || "BASE",
        ).split("|");
        h +=
          `<tr><th class="row" scope="row">${esc(RN(r))}</th>` +
          specs
            .map((sp) => {
              const row = cell(r, sp);
              if (!row) return `<td class="na">·</td>`;
              const ring = picks.includes(sp)
                ? ";box-shadow:inset 0 0 0 2px var(--ink)"
                : "";
              return `<td tabindex="0" data-k="${esc(r + "|" + cl + "|" + sp)}" aria-haspopup="dialog" aria-label="${esc(RN(r) + " " + cl + " " + prettySpec(sp) + ": " + m.label + " " + m.fmt(row[m.f]) + ". Open details")}" style="${cellStyle(colour(row[m.f]))}${ring}">${m.fmt(row[m.f])}</td>`;
            })
            .join("") +
          "</tr>";
      });
      if (st.met === "idx") {
        h +=
          `<tr class="foot"><th class="row" scope="row">Class average</th>` +
          specs
            .map((sp) => {
              const x = cs[sp];
              return `<td class="sum" style="${x ? cellStyle(divColor(x[1])) : ""}">${x ? fIdx(x[1]) : ""}</td>`;
            })
            .join("") +
          "</tr>";
      }
    }
    $("atlas-table").innerHTML = h + "</tbody>";
    $("atlas-caption").textContent =
      `Level ${L} · ${TL[tier]} gear · ${m.label} · ${cl} specialisations (outlined = ${D.meta.field === "full" ? "that race's best build" : "the model's pick"})`;
    legend($("atlas-legend"), m.kind, max);
    $("atlas-note").innerHTML = specs
      .map((sp) => {
        const t = aboutText(cl, sp);
        return `<strong>${esc(prettySpec(sp))}</strong>${t ? ": " + esc(t) : ""}`;
      })
      .join(" · ");
  }

  /* tooltip */
  const tip = $("tip");
  function showSpecTip(td, x, y) {
    const k = key();
    const [r, cl, sp] = td.dataset.k.split("|");
    const v = SP && SP.variants[td.dataset.k];
    const row = v && v[k];
    if (!row) return;
    const line = (a, b) => `<dt>${a}</dt><dd>${b}</dd>`;
    const about = (SP.about || {})[cl + "|" + sp];
    tip.innerHTML =
      `<h4>${esc(RN(r) + " " + cl)} <span style="font:400 11px var(--mono);color:var(--muted)">${esc(prettySpec(sp))}</span></h4><dl>` +
      line("Duel index", fIdx(row[0])) +
      line("Duels won", row[1] == null ? "–" : Math.round(row[1] * 100) + "%") +
      line("PvE kill rate", fIdx(row[2])) +
      line("Max HP", fNum(row[3])) +
      line("Round", row[4] == null ? "–" : row[4].toFixed(2) + " s") +
      line("Melee dmg/s", fNum(row[5])) +
      line("Spell dmg/s", fNum(row[6]) + (row[8] ? " · " + esc(row[8]) : "")) +
      (row[7] ? line("Heal/s", fNum(row[7])) : "") +
      `</dl>` +
      (aboutText(cl, sp)
        ? `<p style="margin:6px 0 0;color:var(--muted);font-size:11.5px">${esc(aboutText(cl, sp))}</p>`
        : "");
    placeTip(x, y);
  }
  function placeTip(x, y) {
    tip.hidden = false;
    const w = tip.offsetWidth,
      hgt = tip.offsetHeight;
    let left = x + 14,
      top = y + 14;
    if (left + w > window.innerWidth - 8) left = x - w - 14;
    if (top + hgt > window.innerHeight - 8) top = y - hgt - 14;
    tip.style.left = Math.max(8, left) + "px";
    tip.style.top = Math.max(8, top) + "px";
  }
  function showTip(td, x, y) {
    if (td.dataset.k.split("|").length === 3) {
      showSpecTip(td, x, y);
      return;
    }
    const k = key();
    const cc = D.combos[td.dataset.k];
    const row = cc && cc.d[k];
    if (!row) return;
    const tier = k.split("|")[1];
    const spec = cc.s && cc.s[k];
    const line = (a, b) => `<dt>${a}</dt><dd>${b}</dd>`;
    tip.innerHTML =
      `<h4>${esc(combo(td.dataset.k))}${specLabel(spec, k) ? ` <span style="font:400 11px var(--mono);color:var(--muted)">${esc(specLabel(spec, k))}</span>` : ""}</h4><dl>` +
      line("Duel index", fIdx(row[F.idx])) +
      line(
        "Duels won",
        row[F.win] == null ? "–" : Math.round(row[F.win] * 100) + "%",
      ) +
      line("PvE kill rate", fIdx(row[F.pve])) +
      line("Max HP", fNum(row[F.hp])) +
      line(
        "Round",
        row[F.round] == null ? "–" : row[F.round].toFixed(2) + " s",
      ) +
      line("Melee dmg/s", fNum(row[F.mel])) +
      line(
        "Spell dmg/s",
        fNum(row[F.spl]) + (row[F.spell] ? " · " + esc(row[F.spell]) : ""),
      ) +
      (row[F.heal]
        ? line(
            "Heal/s",
            fNum(row[F.heal]) + (row[F.hname] ? " · " + esc(row[F.hname]) : ""),
          )
        : "") +
      line("eHP vs melee", fNum(row[F.ehpm])) +
      line("eHP vs spells", fNum(row[F.ehps])) +
      line(
        "Shrug",
        row[F.shrug] == null ? "–" : row[F.shrug].toFixed(1) + "%",
      ) +
      line(
        "Spell save",
        row[F.save] == null ? "–" : row[F.save].toFixed(0) + "%",
      ) +
      (row[F.burst]
        ? line(
            "Opener",
            fNum(row[F.burst]) +
              (row[F.bkind] ? " · " + esc(row[F.bkind]) : ""),
          )
        : "") +
      `</dl>`;
    tip.innerHTML += specLine(td.dataset.k, k, spec);
    placeTip(x, y);
  }
  const atlasT = $("atlas-table");
  function hideTip() {
    tip.hidden = true;
    atlasT
      .querySelectorAll('[aria-describedby="tip"]')
      .forEach((td) => td.removeAttribute("aria-describedby"));
  }
  function preview(td, x, y) {
    hideTip();
    showTip(td, x, y);
    td.setAttribute("aria-describedby", "tip");
  }
  atlasT.addEventListener("pointermove", (e) => {
    if (e.pointerType === "touch") return;
    const td = e.target.closest("td[data-k]");
    if (td) preview(td, e.clientX, e.clientY);
    else hideTip();
  });
  atlasT.addEventListener("pointerleave", hideTip);
  atlasT.addEventListener("focusin", (e) => {
    const td = e.target.closest("td[data-k]");
    if (td) {
      const r = td.getBoundingClientRect();
      preview(td, r.right, r.bottom);
    }
  });
  atlasT.addEventListener("focusout", hideTip);
  function openDetails(td) {
    showTip(td, 0, 0);
    $("cell-detail-content").innerHTML = tip.innerHTML
      .replace("<h4>", '<h3 id="cell-title">')
      .replace("</h4>", "</h3>");
    hideTip();
    $("cell-details").showModal();
  }
  atlasT.addEventListener("click", (e) => {
    const td = e.target.closest("td[data-k]");
    if (td) openDetails(td);
  });
  atlasT.addEventListener("keydown", (e) => {
    const td = e.target.closest("td[data-k]");
    if (td && ["Enter", " "].includes(e.key)) {
      e.preventDefault();
      openDetails(td);
    }
  });
  document.addEventListener("keydown", (e) => {
    if (e.key === "Escape") hideTip();
  });
  $("copy-view").addEventListener("click", async () => {
    const url = new URL(location.href);
    url.hash = "atlas";
    try {
      await navigator.clipboard.writeText(url.href);
      $("share-status").textContent = "View link copied.";
    } catch {
      $("share-status").textContent =
        "Copy this page’s address to share the current view.";
    }
  });

  /* ---------- standing ---------- */
  function renderStanding() {
    const hk = (L) => L + "|" + ht(L);
    const clsAt = (L) =>
      Object.fromEntries(
        ((D.ranks[hk(L)] || {}).classes || []).map((x) => [x[0], x[1]]),
      );
    const raceAt = (L) =>
      Object.fromEntries(
        ((D.ranks[hk(L)] || {}).races || []).map((x) => [x[0], x[2]]),
      );
    const last = LV[LV.length - 1];
    const head =
      "<thead><tr><th></th>" +
      LV.map(
        (L) =>
          `<th scope="col">L${L}<br><span style="color:var(--muted);font-weight:400">${ht(L) === "basic" ? "B" : ht(L) === "good" ? "G" : "E"}</span></th>`,
      ).join("") +
      "</tr></thead>";
    const cls = [...D.classes].sort(
      (a, b) => (clsAt(last)[b] || 0) - (clsAt(last)[a] || 0),
    );
    $("cls-table").innerHTML =
      head +
      "<tbody>" +
      cls
        .map(
          (c) =>
            `<tr><th class="row" scope="row">${esc(c)}</th>` +
            LV.map((L) => {
              const v = clsAt(L)[c];
              return v
                ? `<td style="${cellStyle(divColor(v))}">${fIdx(v)}</td>`
                : `<td class="na">·</td>`;
            }).join("") +
            "</tr>",
        )
        .join("") +
      "</tbody>";
    const races = [...D.races].sort(
      (a, b) => (raceAt(last)[b] || 0) - (raceAt(last)[a] || 0),
    );
    $("race-table").innerHTML =
      head +
      "<tbody>" +
      races
        .map(
          (r) =>
            `<tr><th class="row" scope="row">${esc(RN(r))}<small>${esc((D.side[r] || "").slice(0, 1))}</small></th>` +
            LV.map((L) => {
              const v = raceAt(L)[r];
              return v
                ? `<td style="${cellStyle(divColor(v, 0.6))}">${fMult(v)}</td>`
                : `<td class="na">·</td>`;
            }).join("") +
            "</tr>",
        )
        .join("") +
      "</tbody>";
    legend($("cls-legend"), "div");
    legend($("race-legend"), "fac");
  }

  /* ---------- movers ---------- */
  function renderMovers() {
    const steps = [];
    for (let i = 1; i < LV.length; i++) steps.push(LV[i - 1] + "-" + LV[i]);
    chips(
      $("step-chips"),
      steps.filter((s) => D.jumps[s]).map((s) => [s, s.replace("-", "→")]),
      st.step,
      (v) => {
        st.step = v;
        renderMovers();
      },
    );
    const list = D.jumps[st.step] || [];
    const n = D.jumps[st.step + "#n"] || 0;
    $("movers").innerHTML =
      `<thead><tr><th>Combination</th><th class="num">Change</th><th class="num">Percentile now</th><th>Why</th></tr></thead><tbody>` +
      list
        .map((j) => {
          const m = Math.exp(j.dlog);
          const up = m >= 1;
          return `<tr><td>${esc(combo(j.combo))}</td><td class="num ${up ? "up" : "down"}">${up ? "▲" : "▼"} ${fMult(m)}</td><td class="num">${j.pct == null ? "–" : Math.round(j.pct)}</td><td>${esc((j.causes || []).join("; "))}</td></tr>`;
        })
        .join("") +
      `</tbody><caption style="caption-side:bottom;text-align:left;padding:8px 10px;color:var(--muted);font-size:12.5px">${n} combinations flagged at this step; the eight largest moves shown.</caption>`;
  }

  /* ---------- factors ---------- */
  const FLABEL = {
    hp: "HP (racial Constitution)",
    pulse: "Pulse",
    stats: "Stats",
    resist: "Shrug, saves and resists",
    innate: "Innates",
    size: "Size rules",
    control: "Control and heals",
    pets: "Pets",
  };
  const RACIAL = ["hp", "pulse", "stats", "resist", "innate", "size"];
  function factorKeys(L) {
    return RACIAL.filter((f) => D.decomposition[L] && D.decomposition[L][f]);
  }
  function renderFactorSummary() {
    const Ls = [21, 46, 56].filter((L) => D.decomposition[L]);
    const fk = factorKeys(Ls[Ls.length - 1]);
    $("factor-summary").innerHTML =
      `<thead><tr><th>Factor</th>${Ls.map((L) => `<th class="num">Average move, L${L}</th>`).join("")}<th>Helps most</th><th>Holds back most</th></tr></thead><tbody>` +
      fk
        .map((f) => {
          const d56 = D.decomposition[Ls[Ls.length - 1]][f];
          const byRace = raceMeans(Ls[Ls.length - 1], f);
          const sorted = Object.entries(byRace).sort((a, b) => b[1] - a[1]);
          const top = sorted
            .slice(0, 3)
            .map(([r, v]) => `${esc(RN(r))} ${fMult(v)}`)
            .join(", ");
          const bot = sorted
            .slice(-3)
            .reverse()
            .map(([r, v]) => `${esc(RN(r))} ${fMult(v)}`)
            .join(", ");
          return `<tr><td>${esc(FLABEL[f] || f)}</td>${Ls.map((L) => {
            const d = D.decomposition[L][f];
            return `<td class="num">${d ? fMult(Math.exp(d.mas)) : "–"}</td>`;
          }).join("")}<td>${top}</td><td>${bot}</td></tr>`;
        })
        .join("") +
      `</tbody><caption style="caption-side:bottom;text-align:left;padding:8px 10px;color:var(--muted);font-size:12.5px">"Average move" is the typical size of the change, in either direction, when the factor is switched off. "Helps most" and "Holds back most" are race averages at L${Ls[Ls.length - 1]}.</caption>`;
  }
  function raceMeans(L, f) {
    const by =
      (D.decomposition[L] &&
        D.decomposition[L][f] &&
        D.decomposition[L][f].by) ||
      {};
    const acc = {};
    Object.entries(by).forEach(([ck, s]) => {
      if (s == null) return;
      const r = ck.split("|")[0];
      (acc[r] = acc[r] || []).push(s);
    });
    const out = {};
    Object.entries(acc).forEach(([r, arr]) => {
      out[r] = Math.exp(arr.reduce((a, b) => a + b, 0) / arr.length);
    });
    return out;
  }
  function renderFactorGrid() {
    const Ls = LV.filter((L) => D.decomposition[L]);
    chips(
      $("factor-chips"),
      Ls.map((L) => [L, "L" + L]),
      st.fL,
      (v) => {
        st.fL = +v;
        renderFactorGrid();
      },
    );
    const fk = factorKeys(st.fL);
    const means = Object.fromEntries(fk.map((f) => [f, raceMeans(st.fL, f)]));
    $("factor-table").innerHTML =
      "<thead><tr><th></th>" +
      fk.map((f) => `<th scope="col">${esc(FLABEL[f] || f)}</th>`).join("") +
      "</tr></thead><tbody>" +
      D.races
        .map(
          (r) =>
            `<tr><th class="row" scope="row">${esc(RN(r))}</th>` +
            fk
              .map((f) => {
                const v = means[f][r];
                return v
                  ? `<td style="${cellStyle(divColor(v, 0.6))};min-width:92px">${fMult(v)}</td>`
                  : `<td class="na">·</td>`;
              })
              .join("") +
            "</tr>",
        )
        .join("") +
      "</tbody>";
    $("factor-caption").textContent =
      `Level ${st.fL} · ${TL[ht(st.fL)]} gear · effect of each race's own term`;
    legend($("factor-legend"), "fac");
  }

  /* ---------- halfling warrior ---------- */
  function renderHalfling() {
    const HK = "Halfling|Warrior",
      HU = "Human|Warrior";
    let h = `<thead><tr><th>Level</th><th>Gear</th><th class="num">Index</th><th class="num">Percentile</th><th class="num">Rank of warriors</th><th class="num">Human warrior</th><th>Best warrior</th></tr></thead><tbody>`;
    LV.forEach((L) => {
      const k = L + "|" + ht(L);
      const hw = D.combos[HK] && D.combos[HK].d[k];
      if (!hw) return;
      const wars = D.races
        .map((r) => [
          r,
          D.combos[r + "|Warrior"] && D.combos[r + "|Warrior"].d[k],
        ])
        .filter((x) => x[1])
        .sort((a, b) => b[1][F.idx] - a[1][F.idx]);
      const rank = wars.findIndex((x) => x[0] === "Halfling") + 1;
      const hu = D.combos[HU] && D.combos[HU].d[k];
      const pct = D.ranks[k] && D.ranks[k].pct ? D.ranks[k].pct[HK] : null;
      h += `<tr><td class="mono">L${L}</td><td>${TL[ht(L)]}</td><td class="num">${fIdx(hw[F.idx])}</td><td class="num">${pct == null ? "–" : Math.round(pct)}</td><td class="num">${rank} of ${wars.length}</td><td class="num">${hu ? fIdx(hu[F.idx]) : "–"}</td><td>${esc(RN(wars[0][0]))} ${fIdx(wars[0][1][F.idx])}</td></tr>`;
    });
    $("hw-table").innerHTML = h + "</tbody>";
    const Ls = LV.filter((L) => D.decomposition[L]);
    chips(
      $("hw-chips"),
      Ls.map((L) => [L, "L" + L]),
      st.hwL,
      (v) => {
        st.hwL = +v;
        renderHalflingFactors();
      },
    );
    renderHalflingFactors();
  }
  function renderHalflingFactors() {
    const HK = "Halfling|Warrior";
    chips(
      $("hw-chips"),
      LV.filter((L) => D.decomposition[L]).map((L) => [L, "L" + L]),
      st.hwL,
      (v) => {
        st.hwL = +v;
        renderHalflingFactors();
      },
    );
    const fk = factorKeys(st.hwL);
    $("hw-factors").innerHTML = fk
      .map((f) => {
        const s = D.decomposition[st.hwL][f].by[HK];
        if (s == null) return "";
        const v = Math.exp(s),
          bg = divColor(v, 0.6);
        return `<div style="${cellStyle(bg)}"><b>${esc(FLABEL[f] || f)}</b><span>${fMult(v)}</span></div>`;
      })
      .join("");
    const k = st.hwL + "|" + ht(st.hwL);
    const hw = D.combos[HK] && D.combos[HK].d[k];
    const hu = D.combos["Human|Warrior"] && D.combos["Human|Warrior"].d[k];
    $("hw-note").textContent =
      hw && hu
        ? `At level ${st.hwL} (${TL[ht(st.hwL)]} gear) the halfling warrior has ${fNum(hw[F.hp])} HP against the human warrior's ${fNum(hu[F.hp])}, swings every ${hw[F.round].toFixed(2)} s against ${hu[F.round].toFixed(2)} s, and deals ${fNum(hw[F.mel])} melee damage a second against ${fNum(hu[F.mel])}. Each tile is the multiplier its own racial term applies to its index.`
        : "";
  }

  /* ---------- globes ---------- */
  function renderGlobes() {
    const Ls = Object.keys(D.globes)
      .map(Number)
      .sort((a, b) => a - b);
    if (!Ls.includes(st.gL)) st.gL = Ls[Ls.length - 1];
    chips(
      $("globe-chips"),
      Ls.map((L) => [L, "L" + L]),
      st.gL,
      (v) => {
        st.gL = +v;
        renderGlobes();
      },
    );
    const base = Object.fromEntries(
      ((D.ranks[st.gL + "|endgame"] || {}).classes || []).map((x) => [
        x[0],
        x[1],
      ]),
    );
    const ng = Object.fromEntries(
      (D.globes[st.gL].classes || []).map((x) => [x[0], x[1]]),
    );
    const rows = D.classes
      .filter((c) => base[c] && ng[c])
      .map((c) => [c, base[c], ng[c], ng[c] / base[c]])
      .sort((a, b) => b[3] - a[3]);
    $("globe-table").innerHTML =
      `<thead><tr><th>Class</th><th class="num">End-game index</th><th class="num">Without globe gear</th><th class="num">Change</th></tr></thead><tbody>` +
      rows
        .map(
          ([c, b, n, r]) =>
            `<tr><td>${esc(c)}</td><td class="num"><span class="pill" style="${cellStyle(divColor(b))}">${fIdx(b)}</span></td><td class="num"><span class="pill" style="${cellStyle(divColor(n))}">${fIdx(n)}</span></td><td class="num ${r >= 1 ? "up" : "down"}">${r >= 1 ? "▲" : "▼"} ${fMult(r)}</td></tr>`,
        )
        .join("") +
      "</tbody>";
  }

  /* ---------- pvp ---------- */
  function renderPvp() {
    const keys = Object.keys(D.pvp).filter((k) => !k.includes("with_pets"));
    const label = (k) => {
      const [L, t] = k.split("|");
      return "L" + L + " " + TL[t];
    };
    $("pvp-rho").innerHTML =
      `<thead><tr><th>Model state</th><th class="num">Per combination</th><th class="num">Per race</th><th class="num">Per class</th></tr></thead><tbody>` +
      Object.keys(D.pvp)
        .map((k) => {
          const v = D.pvp[k];
          return `<tr><td>${esc(label(k.replace("|with_pets", ""))) + (k.includes("with_pets") ? " + pets" : "")}</td><td class="num">${v.rho_combo.toFixed(2)}</td><td class="num">${v.rho_race.toFixed(2)}</td><td class="num">${v.rho_class.toFixed(2)}</td></tr>`;
        })
        .join("") +
      `</tbody><caption style="caption-side:bottom;text-align:left;padding:8px 10px;color:var(--muted);font-size:12.5px">Spearman rank correlation between the model's duel index and normalised PvP pick rates. 1 would be perfect agreement, 0 none.</caption>`;
    chips(
      $("pvp-chips"),
      keys.map((k) => [k, label(k)]),
      st.pvp,
      (v) => {
        st.pvp = v;
        renderPvp();
      },
    );
    const rows = (D.pvp[st.pvp] || {}).race_rows || [];
    const W = 640,
      H = 380,
      m = { l: 52, r: 18, t: 14, b: 44 };
    const xs = rows.map((r) => r[4]).filter((v) => v > 0),
      ysAll = rows.map((r) => r[3]).filter((v) => v > 0);
    const xmin = Math.log10(Math.min(...xs) / 1.3),
      xmax = Math.log10(Math.max(...xs) * 1.3);
    const yfloor = Math.min(...ysAll) / 1.6;
    const ymin = Math.log10(yfloor),
      ymax = Math.log10(Math.max(...ysAll) * 1.3);
    const X = (v) =>
      m.l + ((Math.log10(v) - xmin) / (xmax - xmin)) * (W - m.l - m.r);
    const Y = (v) =>
      H -
      m.b -
      ((Math.log10(Math.max(v, yfloor)) - ymin) / (ymax - ymin)) *
        (H - m.t - m.b);
    const tickVals = [0.1, 0.2, 0.3, 0.5, 1, 2, 3, 5, 10, 20];
    const xt = tickVals.filter(
      (v) => Math.log10(v) >= xmin && Math.log10(v) <= xmax,
    );
    const yt = tickVals.filter(
      (v) => Math.log10(v) >= ymin && Math.log10(v) <= ymax,
    );
    const sideCol = {
      good: "var(--s1)",
      evil: "var(--s2)",
      neutral: "var(--s3)",
    };
    let s = `<svg viewBox="0 0 ${W} ${H}" width="100%" role="img" aria-label="Scatter of race pick rate against model strength">`;
    xt.forEach((v) => {
      s += `<line x1="${X(v)}" x2="${X(v)}" y1="${m.t}" y2="${H - m.b}" stroke="var(--rule)" stroke-width="1"/><text x="${X(v)}" y="${H - m.b + 16}" text-anchor="middle" font-size="11" fill="var(--muted)">${v}</text>`;
    });
    yt.forEach((v) => {
      s += `<line x1="${m.l}" x2="${W - m.r}" y1="${Y(v)}" y2="${Y(v)}" stroke="var(--rule)" stroke-width="1"/><text x="${m.l - 8}" y="${Y(v) + 4}" text-anchor="end" font-size="11" fill="var(--muted)">${v}</text>`;
    });
    if (Math.log10(1) >= ymin && Math.log10(1) <= ymax)
      s += `<line x1="${m.l}" x2="${W - m.r}" y1="${Y(1)}" y2="${Y(1)}" stroke="var(--rule-strong)" stroke-width="1"/>`;
    s += `<text x="${(m.l + W - m.r) / 2}" y="${H - 8}" text-anchor="middle" font-size="11.5" fill="var(--ink-2)">Model race strength (duel index, log scale)</text>`;
    s += `<text transform="translate(14 ${(m.t + H - m.b) / 2}) rotate(-90)" text-anchor="middle" font-size="11.5" fill="var(--ink-2)">Pick rate (1 = expected)</text>`;
    // Direct labels only where they fit: the most-picked races claim space first, a
    // label that would collide moves to the left of its dot or is left to the tooltip.
    const placed = [];
    const clear = (x, y, w) =>
      placed.every(
        (b) => x + w < b.x || x > b.x + b.w || Math.abs(y - b.y) >= 11,
      );
    [...rows]
      .sort((a, b) => b[1] - a[1])
      .forEach((r) => {
        const [race, picks, legal, rate, idx] = r;
        if (!(idx > 0)) return;
        const cx = X(idx),
          cy = Y(rate);
        const side = D.side[race] || "neutral";
        const name = RN(race),
          w = name.length * 6.4,
          ty = cy + 4;
        let label = "";
        if (cx + 8 + w < W - 2 && clear(cx + 8, ty, w)) {
          placed.push({ x: cx + 8, y: ty, w });
          label = `<text x="${cx + 8}" y="${ty}" font-size="10.5" fill="var(--ink-2)">${esc(name)}</text>`;
        } else if (cx - 8 - w > m.l && clear(cx - 8 - w, ty, w)) {
          placed.push({ x: cx - 8 - w, y: ty, w });
          label = `<text x="${cx - 8}" y="${ty}" text-anchor="end" font-size="10.5" fill="var(--ink-2)">${esc(name)}</text>`;
        }
        s += `<g><title>${esc(name)}: ${picks} picks over ${legal} legal classes, pick rate ${rate.toFixed(2)}${rate === 0 ? " (no picks; drawn at the floor)" : ""}, model index ${fIdx(idx)}</title><circle cx="${cx}" cy="${cy}" r="5" fill="${sideCol[side]}" stroke="var(--surface)" stroke-width="2"/>${label}</g>`;
      });
    s +=
      `</svg><div class="legend" style="gap:16px;padding:4px 6px 2px">` +
      ["good", "evil", "neutral"]
        .map(
          (sd) =>
            `<span style="display:flex;align-items:center;gap:6px"><svg width="10" height="10"><circle cx="5" cy="5" r="5" fill="${sideCol[sd]}"/></svg>${sd}</span>`,
        )
        .join("") +
      `</div>`;
    s +=
      `<details style="margin-top:8px"><summary class="caption" style="cursor:pointer">Show as a table</summary><table class="plain"><thead><tr><th>Race</th><th>Side</th><th class="num">Picks</th><th class="num">Legal classes</th><th class="num">Pick rate</th><th class="num">Model index</th></tr></thead><tbody>` +
      [...rows]
        .sort((a, b) => b[3] - a[3])
        .map(
          (r) =>
            `<tr><td>${esc(RN(r[0]))}</td><td>${esc(D.side[r[0]] || "")}</td><td class="num">${r[1]}</td><td class="num">${r[2]}</td><td class="num">${r[3].toFixed(2)}</td><td class="num">${fIdx(r[4])}</td></tr>`,
        )
        .join("") +
      `</tbody></table></details>`;
    $("pvp-plot").innerHTML = s;
  }

  /* ---------- specialisations table ---------- */
  /* In the full field, count the races for which this build is best at L56 end-game, crediting
   * every build in an exact tie (to the three decimals the page carries). */
  function bestForText(cl, sp, e) {
    if (D.meta.field !== "full") return e ? e[7] + " of " + e[2] : "–";
    const ck = "56|endgame";
    let n = 0,
      m = 0;
    D.races.forEach((r) => {
      const rows = (SPECS_BY_CLASS[cl] || [])
        .map((s2) => [
          s2,
          ((SP.variants[r + "|" + cl + "|" + s2] || {})[ck] || [])[0],
        ])
        .filter((x) => x[1] != null);
      if (!rows.length) return;
      m++;
      const top = Math.max(...rows.map((x) => x[1]));
      if (
        rows.some(
          (x) => x[0] === sp && Math.abs(x[1] - top) <= 1e-9 * Math.max(1, top),
        )
      )
        n++;
    });
    return m ? n + " of " + m : "–";
  }
  function renderSpecs() {
    if (!SP) {
      $("specs").hidden = true;
      return;
    }
    const cols = [
      ["31|basic", "L31 Basic"],
      ["46|good", "L46 Good"],
      ["56|endgame", "L56 End-game"],
    ];
    const at = (ck, cl, sp) =>
      ((SP.class_spec[ck] || {})[cl] || []).find((x) => x[0] === sp);
    let h = `<thead><tr><th>Class</th><th>Specialisation</th><th>What it changes</th>${cols.map(([, l]) => `<th class="num">${l}</th>`).join("")}<th>Best race, L56</th><th>Worst race, L56</th><th class="num">${D.meta.field === "full" ? "Best build for, L56" : "Picked, L56"}</th></tr></thead><tbody>`;
    let widest = null;
    D.classes.forEach((cl) => {
      const sps = SPECS_BY_CLASS[cl] || [];
      if (!sps.length) return;
      const ordered = [...sps].sort(
        (a, b) =>
          ((at("56|endgame", cl, b) || [])[1] || 0) -
          ((at("56|endgame", cl, a) || [])[1] || 0),
      );
      const vals = ordered
        .map((sp) => (at("56|endgame", cl, sp) || [])[1])
        .filter((v) => v > 0);
      if (vals.length > 1) {
        const spread = Math.max(...vals) / Math.min(...vals);
        if (!widest || spread > widest[1]) widest = [cl, spread];
      }
      ordered.forEach((sp, i) => {
        const about = (SP.about || {})[cl + "|" + sp];
        const e = at("56|endgame", cl, sp);
        h +=
          `<tr${i === 0 ? ' class="grp"' : ""}><td>${i === 0 ? esc(cl) : ""}</td><td class="spec-name">${esc(prettySpec(sp))}</td><td class="spec-about">${esc(aboutText(cl, sp))}</td>` +
          cols
            .map(([ck]) => {
              const x = at(ck, cl, sp);
              return `<td class="num">${x ? `<span class="pill" style="${cellStyle(divColor(x[1]))}">${fIdx(x[1])}</span>` : "–"}</td>`;
            })
            .join("") +
          `<td>${e ? esc(RN(e[3])) + " " + fIdx(e[4]) : "–"}</td><td>${e ? esc(RN(e[5])) + " " + fIdx(e[6]) : "–"}</td><td class="num">${bestForText(cl, sp, e)}</td></tr>`;
      });
    });
    $("spec-table").innerHTML = h + "</tbody>";
    // How often the headline's pick is beaten by another specialisation of the same combination.
    let cells = 0,
      beaten = 0;
    const keys = new Set();
    Object.values(SP.variants).forEach((cellsByKey) =>
      Object.keys(cellsByKey).forEach((ck) => keys.add(ck)),
    );
    Object.entries(D.combos).forEach(([ck, cc]) => {
      const [r, cl] = ck.split("|");
      keys.forEach((k) => {
        const pick = (cc.s || {})[k];
        const pv = pick && SP.variants[r + "|" + cl + "|" + pick];
        const prow = pv && pv[k];
        if (!prow) return;
        cells++;
        const bv = bestVariant(r, cl, k);
        if (bv && bv[1][0] > prow[0] * 1.0005) beaten++;
      });
    });
    $("spec-note").textContent =
      'Specialisations within each class are ordered by their level-56 end-game index. "Best build for" counts the races for which the headline uses this specialisation.' +
      (widest
        ? ` The widest gap between specialisations of one class at L56 end-game is in ${widest[0]} (×${widest[1].toFixed(1)}).`
        : "") +
      (cells && beaten
        ? ` In ${beaten.toLocaleString()} of ${cells.toLocaleString()} cells (${Math.round((beaten / cells) * 100)}%), another specialisation scores higher than the one the headline uses, because the headline chooses against two reference opponents rather than the whole field. The atlas's "Best specialisation" view shows those cells.`
        : "") +
      (D.meta.field === "full"
        ? ' "Best build for" credits every build in an exact tie; ties usually mean the builds differ only in mechanics the model does not simulate (flurry, mounted combat, songs).'
        : " Differences the model does not simulate (flurry, mounted combat, songs) make some specialisations tie.") +
      " Specialisations that change weapon skills or dual wielding get their own gear kit; for Duergar reavers at end-game that kit is poor, which flatters staying unspecialised.";
  }

  /* ---------- strongest and weakest builds (full field only) ---------- */
  function renderBuilds() {
    const VR = D.variantRanks;
    if (!VR) return;
    const cols = [
      ["31|basic", "L31 Basic"],
      ["46|good", "L46 Good"],
      ["56|endgame", "L56 End-game"],
    ].filter(([ck]) => VR[ck]);
    if (!cols.length) return;
    const name = (vk) => {
      const [r, c, sp] = vk.split("|");
      return `${esc(RN(r))} ${esc(c.replace("/", " / "))} <span style="color:var(--muted)">${esc(prettySpec(sp))}</span>`;
    };
    const cellOf = (e) =>
      e
        ? `<td>${name(e[0])}</td><td class="num"><span class="pill" style="${cellStyle(divColor(e[1]))}">${fIdx(e[1])}</span></td>`
        : "<td></td><td></td>";
    const n = 10;
    let h = `<thead><tr><th class="num">#</th>${cols.map(([, l]) => `<th colspan="2">${l}</th>`).join("")}</tr></thead><tbody>`;
    for (let i = 0; i < n; i++)
      h += `<tr><td class="num">${i + 1}</td>${cols.map(([ck]) => cellOf((VR[ck].top25 || [])[i])).join("")}</tr>`;
    h += `<tr class="grp"><td colspan="${cols.length * 2 + 1}" style="color:var(--muted);font-size:12px">Weakest, lowest last</td></tr>`;
    for (let i = n - 1; i >= 0; i--) {
      const counts = cols.map(([ck]) => (VR[ck].bottom25 || []).length);
      h += `<tr><td class="num">${Math.max(...counts) ? "−" + (i + 1) : ""}</td>${cols.map(([ck]) => cellOf((VR[ck].bottom25 || [])[i])).join("")}</tr>`;
    }
    $("builds-table").innerHTML = h + "</tbody>";
    $("builds-panel").hidden = false;
  }

  /* ---------- multiclass (Human/Orc primary/secondary builds) ---------- */
  /* Which class's gear kit each tier uses ("primary"/"secondary" per tier), or a plain label. */
  function kitText(bk, pair) {
    const kit = ((D.multi && D.multi.kits) || {})[bk];
    if (!kit || typeof kit !== "object") return String(kit || "");
    const [pri, sec] = pair.split("/");
    return TIERS.filter((t) => kit[t])
      .map(
        (t) =>
          `${TL[t]}: ${kit[t] === "primary" ? pri : kit[t] === "secondary" ? sec : kit[t]}`,
      )
      .join(" · ");
  }
  function renderMulti() {
    const MU = D.multi;
    if (!MU) return;
    $("multi").hidden = false;
    const meta = MU.meta || {};
    $("multi-intro").textContent =
      meta.summary ||
      "Humans and Orcs can take a second class. Each multiclass build is scored against every other build, single-class and multiclass alike, and compared with the same race's best single-class build.";
    const points = [
      ["21|basic", "L21 Basic"],
      ["31|basic", "L31 Basic"],
      ["46|good", "L46 Good"],
      ["50|good", "L50 Good"],
      ["56|endgame", "L56 End-game"],
    ].filter(([ck]) => Object.values(MU.builds).some((cells) => cells[ck]));
    const at56 = (bk) => ((MU.builds[bk] || {})["56|endgame"] || [])[0] || 0;
    const keys = Object.keys(MU.builds).sort((a, b) => at56(b) - at56(a));
    const vs = (MU.vs_single || {})["56|endgame"] || {};
    const ratio = (a, b) => (a > 0 && b > 0 ? a / b : null);
    const pill = (v) =>
      v == null
        ? "–"
        : `<span class="pill" style="${cellStyle(divColor(v))}">${fIdx(v)}</span>`;
    const change = (r) =>
      r == null
        ? "–"
        : `<span class="${r >= 1 ? "up" : "down"}">${r >= 1 ? "▲" : "▼"} ${fMult(r)}</span>`;
    let h = `<thead><tr><th>Race</th><th>Primary / secondary</th><th>Gear kit</th>${points.map(([, l]) => `<th class="num">${l}</th>`).join("")}<th class="num">vs same race, same primary, L56</th><th class="num">vs race's best single class, L56</th></tr></thead><tbody>`;
    keys.forEach((bk) => {
      const [race, pair] = bk.split("|");
      const v = vs[bk];
      h +=
        `<tr><td>${esc(RN(race))}</td><td class="spec-name">${esc(pair.replace("/", " / "))}</td><td class="spec-about">${esc(kitText(bk, pair))}</td>` +
        points
          .map(
            ([ck]) =>
              `<td class="num">${pill(((MU.builds[bk] || {})[ck] || [])[0])}</td>`,
          )
          .join("") +
        `<td class="num">${change(v ? ratio(v[0], v[1]) : null)}</td><td class="num">${change(v ? ratio(v[0], v[2]) : null)}</td></tr>`;
    });
    $("multi-table").innerHTML = h + "</tbody>";
    const better = keys.filter((bk) => vs[bk] && vs[bk][0] > vs[bk][2]).length;
    $("multi-note").textContent =
      `${keys.length} multiclass builds, ordered by their level-56 end-game index. At that level ${better} of them beat their race's best single-class build.` +
      (meta.levels_note ? " " + meta.levels_note : "");
  }

  function renderAll() {
    renderAtlas();
    renderSpecs();
    renderBuilds();
    renderMulti();
    renderStanding();
    renderMovers();
    renderFactorSummary();
    renderFactorGrid();
    renderHalfling();
    renderGlobes();
    renderPvp();
  }
  // Queries keep the atlas state separate from section anchors. Old hash links still work.
  const params = new URLSearchParams(
    location.search || location.hash.replace(/^#/, ""),
  );
  if (LV.includes(+params.get("L"))) st.L = +params.get("L");
  if (TIERS.includes(params.get("tier"))) {
    st.tier = params.get("tier");
    st.follow = false;
  }
  if (params.get("follow") === "1") st.follow = true;
  if (Object.hasOwn(MET, params.get("met"))) st.met = params.get("met");
  if (Object.hasOwn(SPECS_BY_CLASS, params.get("spec")))
    st.focus = params.get("spec");
  metricSel.value = st.met;
  $("follow").checked = st.follow;
  focusSel.value = st.focus;

  function syncViewUrl() {
    const url = new URL(location.href);
    url.searchParams.set("L", st.L);
    url.searchParams.set("tier", st.follow ? ht(st.L) : st.tier);
    url.searchParams.set("follow", st.follow ? "1" : "0");
    url.searchParams.set("met", st.met);
    if (st.focus) url.searchParams.set("spec", st.focus);
    else url.searchParams.delete("spec");
    if (url.hash.startsWith("#L=")) url.hash = "atlas";
    history.replaceState(null, "", url);
    $("share-status").textContent = "";
  }

  renderAll();
  document.querySelectorAll(".atlas-content .scroll").forEach((el) => {
    el.tabIndex = 0;
    el.setAttribute("role", "region");
    el.setAttribute(
      "aria-label",
      (el.closest("section").querySelector("h2")?.textContent || "Atlas") +
        " — scrollable table",
    );
  });
}

const status = document.getElementById("atlas-status");
try {
  const response = await fetch(
    new URL("../power-atlas/data.json", import.meta.url),
  );
  if (!response.ok)
    throw new Error(`Snapshot request failed: ${response.status}`);
  initAtlas(await response.json());
  status.hidden = true;
} catch (error) {
  console.error("Power Atlas:", error);
  status.textContent =
    "The interactive tables could not load. Reload this page to try again, or download the snapshot above. The report remains readable below.";
  status.setAttribute("role", "alert");
}
