// Mark the nav link whose section sits in the middle of the viewport.
const links = [...document.querySelectorAll("nav a")];
const sections = links
  .map((a) => {
    const href = a.getAttribute("href") || "";
    const hash = href.includes("#") ? href.slice(href.indexOf("#")) : "";
    return hash.length > 1 ? document.querySelector(hash) : null;
  })
  .filter(Boolean);

const watch = new IntersectionObserver((entries) => {
  entries.forEach((entry) => {
    if (!entry.isIntersecting) return;
    const id = "#" + entry.target.id;
    links.forEach((a) => a.classList.toggle("on", a.getAttribute("href") === id));
  });
}, { rootMargin: "-40% 0px -50% 0px" });

sections.forEach((section) => watch.observe(section));
if (document.body.dataset.note !== undefined)
  document.querySelector('nav a[href$="#notes"]')?.classList.add("on");
