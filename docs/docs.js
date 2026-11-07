(function () {
  var pages = [
    { id: "overview", title: "Overview", icon: "book" },
    { id: "signal", title: "Signal path", icon: "sine-wave" },
    { id: "pages", title: "Pages", icon: "pages" },
    { id: "sequencer", title: "Sequencer", icon: "midi" },
    { id: "knobs", title: "Knobs", icon: "settings" },
    { id: "lfos", title: "LFOs", icon: "square-wave" },
    { id: "patches", title: "Patches", icon: "folder" },
    { id: "effects", title: "Effects", icon: "spark" },
    { id: "eq", title: "EQ", icon: "eq" },
    { id: "midi", title: "MIDI", icon: "midi" },
    { id: "spec", title: "Spec", icon: "page", group: "Internals", src: "SPEC.md" },
    { id: "architecture", title: "Architecture", icon: "cube", group: "Internals", src: "ARCHITECTURE.md" },
    { id: "dsp", title: "DSP notes", icon: "cpu", group: "Internals", src: "DSP_NOTES.md" },
    { id: "credits", title: "Credits", icon: "info", group: "About" }
  ];

  var nav = document.getElementById("nav");
  var article = document.getElementById("doc");
  var lastGroup = "";

  pages.forEach(function (page) {
    if (page.group && page.group !== lastGroup) {
      lastGroup = page.group;
      var heading = document.createElement("p");
      heading.className = "nav-group";
      heading.textContent = page.group;
      nav.appendChild(heading);
    }
    var link = document.createElement("a");
    link.href = "#" + page.id;
    var icon = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    icon.setAttribute("class", "icon");
    icon.setAttribute("aria-hidden", "true");
    var use = document.createElementNS("http://www.w3.org/2000/svg", "use");
    use.setAttribute("href", "icons.svg#" + page.icon);
    icon.appendChild(use);
    var label = document.createElement("span");
    label.textContent = page.title;
    link.appendChild(icon);
    link.appendChild(label);
    link.dataset.id = page.id;
    nav.appendChild(link);
  });
  if (window.docsIconsReady) window.docsIconsReady();

  function known(id) {
    return pages.some(function (page) { return page.id === id; });
  }

  function currentId() {
    var id = (location.hash || "#overview").replace(/^#/, "");
    return known(id) ? id : "overview";
  }

  function rewrite(root) {
    root.querySelectorAll("a[href]").forEach(function (link) {
      var href = link.getAttribute("href") || "";
      var hashMd = href.match(/^(?:\.\/)?([a-z0-9-]+)\.md(#[a-z0-9-]*)?$/);
      if (hashMd && known(hashMd[1])) link.setAttribute("href", "#" + hashMd[1]);
      var note = href.match(/notes\.html\?doc=([A-Z0-9_.]+)/i);
      if (note) {
        var map = {
          "SPEC.md": "spec",
          "ARCHITECTURE.md": "architecture",
          "DSP_NOTES.md": "dsp",
          "MANUAL.md": "overview"
        };
        var dest = map[note[1]];
        if (dest) link.setAttribute("href", "#" + dest);
      }
    });
  }

  function show() {
    var id = currentId();
    if ((location.hash || "#overview").replace(/^#/, "") !== id) {
      history.replaceState(null, "", "#" + id);
    }
    nav.querySelectorAll("a").forEach(function (link) {
      var on = link.dataset.id === id;
      if (on) link.setAttribute("aria-current", "page");
      else link.removeAttribute("aria-current");
    });
    var page = pages.filter(function (item) { return item.id === id; })[0];
    document.title = page.title + " | TEW03";
    article.innerHTML = "<p class=\"muted\">Loading…</p>";
    var file = page.src || ("md/" + id + ".md");
    fetch(file)
      .then(function (res) {
        if (!res.ok) throw new Error(String(res.status));
        return res.text();
      })
      .then(function (md) {
        article.innerHTML = marked.parse(md);
        rewrite(article);
      })
      .catch(function () {
        article.innerHTML = "<p>Could not load this page.</p>";
      });
  }

  window.addEventListener("hashchange", show);
  show();
})();
