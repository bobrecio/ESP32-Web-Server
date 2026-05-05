const r = document.getElementById("r");
const g = document.getElementById("g");
const b = document.getElementById("b");
const brightness = document.getElementById("brightness");
const colorPicker = document.getElementById("colorPicker");
const preview = document.getElementById("preview");

let sendTimer = null;
let requestInFlight = false;
let pendingSend = false;

function updateLabels() {
  document.getElementById("rVal").textContent = r.value;
  document.getElementById("gVal").textContent = g.value;
  document.getElementById("bVal").textContent = b.value;
  document.getElementById("brightnessVal").textContent = brightness.value;

  preview.style.background = `rgb(${r.value}, ${g.value}, ${b.value})`;
}

function buildColorUrl() {
  return `/api/color?r=${r.value}&g=${g.value}&b=${b.value}&brightness=${brightness.value}`;
}

async function sendColorNow() {
  if (requestInFlight) {
    pendingSend = true;
    return;
  }

  requestInFlight = true;
  pendingSend = false;

  try {
    await fetch(buildColorUrl(), { cache: "no-store" });
  } catch (err) {
    console.error(err);
  }

  requestInFlight = false;

  if (pendingSend) {
    sendColorNow();
  }
}

function sendColor() {
  updateLabels();

  clearTimeout(sendTimer);
  sendTimer = setTimeout(sendColorNow, 50);
}

function preset(rv, gv, bv) {
  r.value = rv;
  g.value = gv;
  b.value = bv;

  colorPicker.value = "#" +
    Number(rv).toString(16).padStart(2, "0") +
    Number(gv).toString(16).padStart(2, "0") +
    Number(bv).toString(16).padStart(2, "0");

  sendColor();
}

function colorPickerChanged() {
  const hex = colorPicker.value;

  r.value = parseInt(hex.slice(1, 3), 16);
  g.value = parseInt(hex.slice(3, 5), 16);
  b.value = parseInt(hex.slice(5, 7), 16);

  sendColor();
}

async function loadDeviceInfo() {
  const res = await fetch("/api/info", { cache: "no-store" });
  const data = await res.json();

  document.getElementById("deviceInfo").textContent =
    JSON.stringify(data, null, 2);
}

async function loadFiles() {
  const res = await fetch("/api/files", { cache: "no-store" });
  const files = await res.json();

  const html = files.map(file => `
    <div class="file-row">
      <span>${file.name}</span>
      <span>${file.sizeFormatted}</span>
    </div>
  `).join("");

  document.getElementById("fileList").innerHTML = html || "No files found.";
}

async function rebootDevice() {
  const ok = confirm("Reboot the ESP32-S3?");
  if (!ok) return;

  document.getElementById("actionStatus").textContent = "Rebooting...";

  try {
    await fetch("/api/reboot", { cache: "no-store" });
    document.getElementById("actionStatus").textContent =
      "Reboot requested. Refresh the page in a few seconds.";
  } catch (err) {
    document.getElementById("actionStatus").textContent =
      "Device is rebooting or temporarily unavailable.";
  }
}

[r, g, b, brightness].forEach(el => {
  el.addEventListener("input", sendColor);
});

colorPicker.addEventListener("input", colorPickerChanged);

updateLabels();
loadDeviceInfo();
loadFiles();

setInterval(loadDeviceInfo, 5000);
