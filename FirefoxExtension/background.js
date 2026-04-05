// 纯 HTTP POST 版本 —— 最简单、最稳定、最适合你
function sendMessage(msg) {
    // 直接发 POST 请求，没有任何多余代码
    fetch("http://127.0.0.1:8899", {
        method: "POST",
        body: msg
    })
    .then(res => res.text())
    .then(data => {
        console.log("✅ 收到服务器回复：", data);
    })
    .catch(err => {
        console.error("❌ 发送失败：", err);
    });
}

// 测试发送
sendMessage("Hello 守护进程！");
