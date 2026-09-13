<script setup lang="ts">
import { onMounted, ref, nextTick } from "vue";
import { Terminal } from "xterm";
import "xterm/css/xterm.css";
import MudChatPanel from "/src/components/mud/MudChatPanel.vue";
import { useMudStore } from "/src/stores/mudStore";
import { chatPalette } from "/src/utils/chatPalette";
import frames from "./frames.json";
const props = defineProps<{ recipient: string; alternate: boolean }>();
const terminalContainer = ref<HTMLElement | null>(null);
const store = useMudStore();
store.setAccount("Visual" + props.recipient, []);
const rows = frames.filter(
  (f) => f.recipient === props.recipient && f.case === "selected",
);
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
  chatPalette.forEach(
    ([key, value], index) =>
      (theme[names[index]] =
        css.getPropertyValue("--mud-" + key).trim() || value),
  );
  const terminal = new Terminal({
    cols: 90,
    rows: 12,
    fontSize: 13,
    fontFamily: "Consolas, monospace",
    convertEol: true,
    cursorBlink: false,
    theme,
    allowProposedApi: true,
  });
  terminal.open(terminalContainer.value!);
  terminal.write(
    "\x1b[1;37m" +
      props.recipient +
      " — independent recipient choice\x1b[0m\r\n\r\n",
  );
  for (const row of rows) terminal.write(row.terminalAnsi + "\r\n");
  for (const row of rows) {
    const p = row.packet;
    store.addChatMessage(
      p.channel,
      p.sender,
      p.text,
      undefined,
      undefined,
      p.presentation,
    );
  }
  await nextTick();
  window.releaseReady = (window.releaseReady || 0) + 1;
});
</script>
<template>
  <section class="recipient">
    <h2>
      {{ recipient }} · {{ recipient === "Alice" ? "bright cyan" : "red" }}
    </h2>
    <p class="caption">
      Same content, independent character preferences, protected authored green.
    </p>
    <h3>Terminal · actual ANSI frames</h3>
    <div class="terminal" ref="terminalContainer" />
    <h3>Supported web chat panel</h3>
    <div class="chat"><MudChatPanel /></div>
    <p class="caption">
      One say, one tell, one guild event. Click the formatted line to open its
      conversation.
    </p>
  </section>
</template>
