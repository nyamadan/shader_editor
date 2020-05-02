document.body.addEventListener("paste", (event) => {
  if (event.target !== document.body) {
    return;
  }

  const text = (event.clipboardData || window.clipboardData).getData("text");
  Module.SetClipboardText(text);
  event.preventDefault();
}, false);
