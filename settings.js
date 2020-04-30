Module.arguments = ["-linfo", "./assets"];

const copy = document.createElement("textarea");
copy.id = "copy";
copy.style.display = "none";
document.body.appendChild(copy);

document.body.addEventListener("paste", (event) => {
  if (event.target !== document.body) {
    return;
  }

  const text = (event.clipboardData || window.clipboardData).getData("text");
  Module.SetClipboardText(text);
  event.preventDefault();
}, false);
