<script setup lang="ts">
import { onMounted, nextTick } from "vue";
import { Terminal } from "xterm";
import "xterm/css/xterm.css";
import { chatPalette } from "/src/utils/chatPalette";
import report from "./room-frames.json";

const selected = [
  "animated first",
  "animated after unrelated chat",
  "motion off first",
  "static first",
];
const frames = report.frames.filter((frame) => selected.includes(frame.label));
onMounted(async () => {
  const css = getComputedStyle(document.body);
  const theme: Record<string, string> = {
    background: css.getPropertyValue("--mud-background").trim() || "#1a1a1a",
    foreground: css.getPropertyValue("--mud-foreground").trim() || "#e0e0e0",
  };
  const names = [
    "black",
    "blue",
    "green",
    "cyan",
    "red",
    "magenta",
    "yellow",
    "white",
    "brightBlack",
    "brightBlue",
    "brightGreen",
    "brightCyan",
    "brightRed",
    "brightMagenta",
    "brightYellow",
    "brightWhite",
  ];
  chatPalette.forEach(([key, value], index) => {
    theme[names[index]] = css.getPropertyValue("--mud-" + key).trim() || value;
  });
  for (const [index, frame] of frames.entries()) {
    const terminal = new Terminal({
      cols: 90,
      rows: 12,
      fontSize: 14,
      fontFamily: "Consolas, monospace",
      convertEol: true,
      cursorBlink: false,
      theme,
    });
    terminal.open(document.getElementById("room-" + index)!);
    await new Promise<void>((resolve) =>
      terminal.write(frame.terminalAnsi, resolve),
    );
  }
  await nextTick();
  window.releaseReady = 1;
});
</script>

<template>
  <section class="scenery">
    <article
      v-for="(frame, index) in frames"
      :key="frame.label"
      class="recipient"
    >
      <h2>{{ frame.label }}</h2>
      <p class="caption">
        Identical prose. Authored text and layout remain intact; only eligible
        word colors vary.
      </p>
      <div class="terminal" :id="'room-' + index" />
      <p class="caption">
        Water palette IDs: {{ frame.water.join(", ") }} · Forest:
        {{ frame.forest.join(", ") }}
      </p>
    </article>
  </section>
</template>

<style scoped>
.scenery {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
}
</style>
