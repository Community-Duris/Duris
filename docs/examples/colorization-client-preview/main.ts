import { createApp } from "vue";
import { createPinia } from "pinia";
import Recipient from "./Recipient.vue";
import Scenery from "./Scenery.vue";
import "/src/assets/main.css";
declare global {
  interface Window {
    releaseReady?: number;
  }
}
const alt = new URLSearchParams(location.search).has("alternate");
const keys = [
  "black",
  "blue",
  "green",
  "cyan",
  "red",
  "magenta",
  "yellow",
  "white",
  "bright-black",
  "bright-blue",
  "bright-green",
  "bright-cyan",
  "bright-red",
  "bright-magenta",
  "bright-yellow",
  "bright-white",
];
const colors = [
  "#000000",
  "#759dff",
  "#81d69a",
  "#8fd8e8",
  "#ff8f87",
  "#dda9f5",
  "#e9d889",
  "#eee7d8",
  "#879397",
  "#a4c0ff",
  "#b4e89c",
  "#b3e9ef",
  "#ffb5a6",
  "#edc8ff",
  "#fff0b0",
  "#ffffff",
];
if (alt) {
  keys.forEach((key, index) =>
    document.body.style.setProperty("--mud-" + key, colors[index]),
  );
  document.body.style.setProperty("--mud-background", "#101820");
  document.body.style.setProperty("--mud-foreground", "#f2eee2");
}
const single = new URLSearchParams(location.search).get("recipient");
const examples = new URLSearchParams(location.search).has("examples");
const scenery = examples || new URLSearchParams(location.search).has("scenery");
if (scenery) {
  createApp(Scenery).mount("#alice");
  document.getElementById("bob")!.remove();
  document.querySelector<HTMLElement>(".grid")!.style.gridTemplateColumns =
    "1fr";
  document.querySelector("header h1")!.textContent = examples
    ? "Duris · protected art and semantic previews"
    : "Duris · coherent scenery frames";
  document.querySelector("header p")!.textContent =
    "Actual xterm rendering of output captured from the live two-character server walkthrough.";
} else {
  if (!single || single === "Alice")
    createApp(Recipient, { recipient: "Alice", alternate: alt })
      .use(createPinia())
      .mount("#alice");
  if (!single || single === "Bob")
    createApp(Recipient, { recipient: "Bob", alternate: alt })
      .use(createPinia())
      .mount("#bob");
}
document.querySelector("#footer")!.textContent =
  (alt ? "Alternate palette" : "Default dark palette") +
  " · Frozen frames; no browser animation clock · Terminal and chat receive one message each · Local validation only";

if (single && !scenery) {
  document.getElementById(single === "Alice" ? "bob" : "alice")!.remove();
  document.querySelector<HTMLElement>(".grid")!.style.gridTemplateColumns =
    "1fr";
}
