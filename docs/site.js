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
