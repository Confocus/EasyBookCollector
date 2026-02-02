// 监听内容脚本发送的双击事件消息
chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  if (request.type === "pageDblClick") {
    console.log("后台脚本接收到网页双击事件：", request.data);
    const dblClickData = request.data;

    // 核心：调用 Native Messaging，直接向 C++ exe 发送数据
    // 第一个参数：Native Messaging 宿主 ID（必须和 manifest.json、宿主清单文件一致）
    chrome.runtime.sendNativeMessage(
      "com.yourcompany.dblclickreceiver",  // 与 manifest.json 中的 ids 一致
      dblClickData,  // 要发送的数据（JSON 对象，浏览器会自动封装成 Native Messaging 格式）
      (response) => {
        // 回调函数：接收 C++ exe 返回的响应
        if (chrome.runtime.lastError) {
          // 通信失败（如 exe 未启动、协议错误）
          console.error("Native Messaging 通信失败：", chrome.runtime.lastError.message);
          sendResponse({
            status: "error",
            message: "与 C++ exe 通信失败：" + chrome.runtime.lastError.message
          });
        } else {
          // 通信成功
          console.log("C++ exe 返回的响应：", response);
          sendResponse({
            status: "success",
            message: "已成功向 C++ exe 发送数据并接收响应",
            nativeResponse: response
          });
        }
      }
    );

    // 异步通信，返回 true 确保 sendResponse 正常回调
    return true;
  }
});
