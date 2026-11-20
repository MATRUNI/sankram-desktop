// 1. Listen for C Push Events
window.myapp.on("server_alert", (data) => {
  const feed = document.getElementById("alert-feed");
  feed.innerHTML = `<strong>Received C Event:</strong><br/>${data.msg} (value: ${data.value})<br/><small>${new Date().toLocaleTimeString()}</small>`;
  feed.style.borderColor = "#238636";
});

// 2. Button: Invoke C Command
document.getElementById("btn-sysinfo").addEventListener("click", async () => {
  const out = document.getElementById("sysinfo-out");
  out.textContent = "Calling C...";
  try {
    const res = await window.myapp.invoke("get_sysinfo");
    out.textContent = JSON.stringify(JSON.parse(res), null, 2);
  } catch (err) {
    out.textContent = "Error: " + err;
  }
});

// 3. Button: Trigger C Alert
document.getElementById("btn-alert").addEventListener("click", async () => {
  try {
    await window.myapp.invoke("trigger_alert");
  } catch (err) {
    console.error(err);
  }
});

