browser.runtime.onMessage.addListener((msg) => {
  if (msg.action === "save-bookmark") {
    document.body.style.border = "5px solid red";
  } else if (msg.action === "") {
    // 其它操作
  }
});
