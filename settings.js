Module.arguments = ["-linfo", "./assets"];

const copy = document.createElement("textarea");
copy.setAttribute("id", "copy");
copy.setAttribute("display", "none");
document.body.appendChild(copy);

document.body.addEventListener("paste", (event) => {
  if (event.target !== document.body) {
    return;
  }

  const text = (event.clipboardData || window.clipboardData).getData("text");
  Module.SetClipboardText(text);
  event.preventDefault();
}, false);
