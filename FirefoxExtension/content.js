browser.runtime.onMessage.addListener((msg) => {
  if (msg.action === "save-bookmark") {
    document.body.style.border = "5px solid red";
    let titleText = document.title;
    //let titleText = document.getElementById("title").textContent;
    console.log("尝试获取Title", titleText);

    browser.runtime.sendMessage({ title: titleText })
      .then(response => {
        console.log("Background 回复:", response);
      })
      .catch(error => {
        console.error("发送消息出错:", error);
      });
  } else if (msg.action === "") {
    // 其它操作
  }
});
