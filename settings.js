Module.arguments = ["-linfo", "./assets"];

const copy = document.createElement("textarea");
copy.id = "copy";
copy.readOnly = true;
copy.setAttribute("style", "width: 0; height: 0;");
document.body.appendChild(copy);

document.body.addEventListener("paste", (event) => {
  if (event.target !== document.body) {
    return;
  }

  const text = (event.clipboardData || window.clipboardData).getData("text");
  Module.SetClipboardText(text);
  event.preventDefault();
}, false);
