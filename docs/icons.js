(function () {
  var ready = false;

  function retarget() {
    if (!ready) return;
    document.querySelectorAll("use").forEach(function (node) {
      var href = node.getAttribute("href") || "";
      var hashAt = href.indexOf("#");
      if (hashAt > 0) node.setAttribute("href", href.slice(hashAt));
    });
  }

  window.docsIconsReady = retarget;

  fetch("icons.svg")
    .then(function (res) { return res.text(); })
    .then(function (text) {
      var wrap = document.createElement("div");
      wrap.innerHTML = text;
      var sprite = wrap.firstElementChild;
      if (!sprite) return;
      sprite.setAttribute("aria-hidden", "true");
      sprite.style.cssText = "position:absolute;width:0;height:0;overflow:hidden";
      document.body.insertBefore(sprite, document.body.firstChild);
      ready = true;
      retarget();
    });
})();
