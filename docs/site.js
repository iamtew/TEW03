// Mark the nav link whose section sits in the middle of the viewport.
const links = [...document.querySelectorAll("nav a")];
const sections = links
  .map((a) => document.querySelector(a.getAttribute("href")))
  .filter(Boolean);

const watch = new IntersectionObserver((entries) => {
  entries.forEach((entry) => {
    if (!entry.isIntersecting) return;
    const id = "#" + entry.target.id;
    links.forEach((a) => a.classList.toggle("on", a.getAttribute("href") === id));
  });
}, { rootMargin: "-40% 0px -50% 0px" });

sections.forEach((section) => watch.observe(section));

const fx = document.getElementById("fx");
const rand = (a, b) => a + Math.random() * (b - a);
for (let i = 0; i < 32; i++) {
  const el = document.createElement("img");
  el.src = "icon.png";
  el.alt = "";
  el.className = "smile";
  el.style.setProperty("--x", rand(0, 92) + "vw");
  el.style.setProperty("--s", rand(24, 110) + "px");
  el.style.setProperty("--b", rand(2, 10) + "px");
  el.style.setProperty("--t", rand(14, 32) + "s");
  el.style.setProperty("--d", -rand(0, 28) + "s");
  el.style.setProperty("--k", rand(1.2, 4) + "s");
  el.style.setProperty("--turn", (Math.random() < 0.5 ? -1 : 1) * rand(220, 720) + "deg");
  fx.appendChild(el);
}
