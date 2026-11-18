"use strict";
// This is remote fixture JavaScript, never an IB implementation layer.
const state = document.getElementById("state");
const confirmControl = document.getElementById("confirm-control");
const createValue = document.getElementById("create-value");
const ambiguous = document.getElementById("ambiguous");
const oneTime = document.getElementById("one-time");
const forgetValue = document.getElementById("forget-value");
const mutationState = document.getElementById("mutation-state");

confirmControl.addEventListener("click", async () => {
  confirmControl.disabled = true;
  try {
    const response = await fetch("/usable-control", {method:"POST", cache:"no-store"});
    if (!response.ok || await response.text() !== "confirmed\n") throw new Error("unconfirmed");
    state.textContent = "Usable action confirmed by server";
    state.dataset.confirmed = "yes";
  } catch (_) {
    state.textContent = "Usable action unconfirmed";
    state.dataset.confirmed = "no";
  }
});

createValue.addEventListener("click", async () => {
  createValue.disabled = true;
  try {
    const response = await fetch("/action", {method:"POST", cache:"no-store"});
    if (response.status === 403) { mutationState.textContent = "Mock verification required"; return; }
    if (response.status === 409) { mutationState.textContent = "One-time value was already delivered"; return; }
    if (!response.ok) throw new Error("unconfirmed");
    oneTime.textContent = await response.text();
    forgetValue.hidden = false;
    mutationState.textContent = "Synthetic value delivered once";
  } catch (_) { mutationState.textContent = "Result uncertain; no automatic retry"; }
});

forgetValue.addEventListener("click", () => {
  oneTime.textContent = "";
  forgetValue.hidden = true;
  mutationState.textContent = "Displayed value forgotten";
});

ambiguous.addEventListener("click", async () => {
  ambiguous.disabled = true;
  mutationState.textContent = "Operation pending";
  try {
    const response = await fetch("/ambiguous", {method:"POST", cache:"no-store"});
    if (response.status === 403) { mutationState.textContent = "Mock verification required"; return; }
    await response.text();
    mutationState.textContent = "Unexpected complete response";
  } catch (_) { mutationState.textContent = "Remote result uncertain; do not replay"; }
});

// DOM interactive precedes this; only the independently observed action proves
// usability. These asynchronous resources must not block that action.
confirmControl.disabled = false;
createValue.disabled = false;
ambiguous.disabled = false;
state.textContent = "Useful control available";
for (let number = 0; number < 4; number += 1) {
  fetch("/asset/" + number, {cache:"no-store"}).then(response => response.arrayBuffer()).catch(() => {});
}
