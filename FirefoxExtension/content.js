browser.runtime.onMessage.addListener((msg) => {
  if (msg.action === "save-bookmark") {
    document.body.style.border = "5px solid red";
    let title = document.getElementById("title").textContent；
    console.log(title);
    browser.runtime.sendMessage({ title: titleText });
    
  } else if (msg.action === "") {
    // 其它操作
  }
});
