#!/usr/bin/env node
// Serve production Vue components and xterm with synthetic release frames.
import fs from "node:fs/promises";
import path from "node:path";
import { createRequire } from "node:module";
import { fileURLToPath, pathToFileURL } from "node:url";

const [clientArgument, frameArgument, roomArgument, portArgument = "4190"] =
  process.argv.slice(2);
if (!clientArgument || !frameArgument) {
  throw new Error(
    "Usage: node scripts/preview_colorization_client.mjs CLIENT_FRONTEND CHAT_FRAMES [LIVE_REPORT] [PORT]",
  );
}
const clientRoot = path.resolve(clientArgument);
const frameFile = path.resolve(frameArgument);
const roomFile = roomArgument ? path.resolve(roomArgument) : null;
const port = Number(portArgument);
if (!Number.isInteger(port) || port < 1024 || port > 65535)
  throw new Error("Invalid preview port");
const templateRoot = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)),
  "../docs/examples/colorization-client-preview",
);
const req = createRequire(path.join(clientRoot, "package.json"));
const { createServer } = await import(pathToFileURL(req.resolve("vite")).href);
const vue = (
  await import(pathToFileURL(req.resolve("@vitejs/plugin-vue")).href)
).default;
const previewRoot = await fs.mkdtemp(
  path.join(clientRoot, "colorization-preview-"),
);
const previewBase = "/" + path.basename(previewRoot);
for (const name of ["index.html", "main.ts", "Recipient.vue", "Scenery.vue"]) {
  const source = await fs.readFile(path.join(templateRoot, name), "utf8");
  await fs.writeFile(
    path.join(previewRoot, name),
    source.replaceAll("@PREVIEW_BASE@", previewBase),
  );
}
await fs.copyFile(frameFile, path.join(previewRoot, "frames.json"));
await fs.writeFile(
  path.join(previewRoot, "room-frames.json"),
  roomFile
    ? await fs.readFile(roomFile, "utf8")
    : JSON.stringify({ frames: [] }),
);
process.chdir(clientRoot); // Resolve the client's Tailwind/PostCSS configuration.
const origin = `http://127.0.0.1:${port}`;
const server = await createServer({
  configFile: false,
  envDir: false,
  root: clientRoot,
  plugins: [
    vue(),
    {
      name: "synthetic-preview-api",
      configureServer(vite) {
        vite.middlewares.use("/api", (_request, response) => {
          response.setHeader("Content-Type", "application/json");
          response.end("{}");
        });
      },
    },
  ],
  optimizeDeps: { entries: [path.join(previewRoot, "index.html")] },
  resolve: { alias: { "@": path.join(clientRoot, "src") } },
  server: { host: "127.0.0.1", port, strictPort: true },
  define: {
    "import.meta.env.VITE_BASE_URL": JSON.stringify("/"),
    "import.meta.env.VITE_API_URL": JSON.stringify(origin),
    "import.meta.env.VITE_WS_URL": JSON.stringify(`ws://127.0.0.1:${port}/ws`),
    "import.meta.env.VITE_STATIC_URL": JSON.stringify(origin),
  },
});
async function close() {
  await server.close();
  // Only remove this invocation's mkdtemp directory inside the explicit client root.
  const resolved = path.resolve(previewRoot);
  if (
    path.dirname(resolved) !== clientRoot ||
    !path.basename(resolved).startsWith("colorization-preview-")
  ) {
    throw new Error("Refusing to remove an unexpected preview directory");
  }
  await fs.rm(resolved, { recursive: true, force: true });
}
let closing = false;
for (const signal of ["SIGINT", "SIGTERM"])
  process.once(signal, async () => {
    if (closing) return;
    closing = true;
    await close();
    process.exit(0);
  });
try {
  await server.listen();
  console.log(`${origin}${previewBase}/index.html`);
  console.log(
    "Add ?alternate=1, ?recipient=Bob, or ?scenery=1. Synthetic local preview only.",
  );
} catch (error) {
  await close();
  throw error;
}
