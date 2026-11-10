const links = [...document.querySelectorAll("#site-nav a[href^='#']")];
const sections = links
  .map((a) => {
    const href = a.getAttribute("href") || "";
    return href.length > 1 ? document.querySelector(href) : null;
  })
  .filter(Boolean);

if (sections.length) {
  const watch = new IntersectionObserver((entries) => {
    entries.forEach((entry) => {
      if (!entry.isIntersecting) return;
      const id = "#" + entry.target.id;
      links.forEach((a) => a.classList.toggle("on", a.getAttribute("href") === id));
    });
  }, { rootMargin: "-40% 0px -50% 0px" });
  sections.forEach((section) => watch.observe(section));
}

const header = document.querySelector("header");
const toggle = document.querySelector(".nav-toggle");
if (header && toggle) {
  toggle.addEventListener("click", () => {
    const open = header.classList.toggle("nav-open");
    toggle.setAttribute("aria-expanded", open ? "true" : "false");
  });
}

const dialog = document.getElementById("dl");
if (dialog) {
  document.querySelectorAll("[data-dl-open]").forEach((btn) => {
    btn.addEventListener("click", () => dialog.showModal());
  });
  document.querySelectorAll("[data-dl-close]").forEach((btn) => {
    btn.addEventListener("click", () => dialog.close());
  });
  dialog.addEventListener("click", (event) => {
    if (event.target === dialog) dialog.close();
  });
}

const shot = document.getElementById("shot");
const shotImg = document.getElementById("shot-full");
if (shot && shotImg) {
  document.querySelectorAll("[data-shot]").forEach((btn) => {
    btn.addEventListener("click", () => {
      shotImg.src = btn.getAttribute("data-shot");
      shotImg.alt = btn.getAttribute("data-shot-alt") || "";
      shot.showModal();
    });
  });
  document.querySelectorAll("[data-shot-close]").forEach((btn) => {
    btn.addEventListener("click", () => shot.close());
  });
  shot.addEventListener("click", (event) => {
    if (event.target !== shotImg) shot.close();
  });
}

// TEW03-<ver>-{win|linux|mac}-{arch}-{installer|vst3|standalone}.{zip|msi}
const FILE = /-(win|linux|mac)-[^-]+-(installer|vst3|standalone)(?:\.zip|\.msi)$/i;
const box = dialog;
if (box) {
  fetch("releases.json")
    .then((r) => (r.ok ? r.json() : null))
    .then((data) => {
      if (!data?.tag || !Array.isArray(data.files)) return;
      const base = `https://github.com/iamtew/TEW03/releases/download/${data.tag}/`;
      for (const name of data.files) {
        const m = name.match(FILE);
        if (!m) continue;
        const a = box.querySelector(`[data-kind="${m[2].toLowerCase()}"] a[data-os="${m[1].toLowerCase()}"]`);
        if (!a) continue;
        a.href = base + encodeURIComponent(name);
        a.hidden = false;
      }
      box.querySelectorAll(".dl-row").forEach((row) => {
        if (row.querySelector("a[href]")) row.hidden = false;
      });
    })
    .catch(() => {});
}

// Native <audio> progress ignores accent-color. Skin it; controls stay if JS is off.
const fmtTime = (sec) => {
  if (!isFinite(sec) || sec < 0) return "0:00";
  const s = Math.floor(sec);
  return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0");
};

document.querySelectorAll(".demo audio").forEach((audio) => {
  const player = document.createElement("div");
  player.className = "player";
  const btn = document.createElement("button");
  btn.type = "button";
  btn.setAttribute("aria-label", "Play");
  btn.textContent = "▶";
  const bar = document.createElement("input");
  bar.type = "range";
  bar.min = "0";
  bar.max = "0";
  bar.step = "any";
  bar.value = "0";
  bar.setAttribute("aria-label", "Seek");
  const time = document.createElement("span");
  time.className = "player-time";
  time.textContent = "0:00";
  audio.classList.add("skinned");
  audio.removeAttribute("controls");
  audio.before(player);
  player.append(btn, bar, time);

  const stamp = () => {
    time.textContent = fmtTime(audio.currentTime) + " / " + fmtTime(audio.duration);
    if (!isFinite(audio.duration) || audio.duration <= 0) return;
    bar.max = String(audio.duration);
    if (document.activeElement !== bar) bar.value = String(audio.currentTime);
  };

  btn.addEventListener("click", () => {
    if (audio.paused) {
      document.querySelectorAll(".demo audio").forEach((other) => {
        if (other !== audio) other.pause();
      });
      audio.play();
    } else {
      audio.pause();
    }
  });
  audio.addEventListener("play", () => {
    btn.textContent = "❚❚";
    btn.setAttribute("aria-label", "Pause");
  });
  audio.addEventListener("pause", () => {
    btn.textContent = "▶";
    btn.setAttribute("aria-label", "Play");
  });
  audio.addEventListener("timeupdate", stamp);
  audio.addEventListener("loadedmetadata", stamp);
  bar.addEventListener("input", () => {
    audio.currentTime = Number(bar.value);
    stamp();
  });
  stamp();
});
