// 一、扩展安装/更新时，创建自定义右键菜单项（原有逻辑不变）
chrome.runtime.onInstalled.addListener(() => {
  chrome.contextMenus.create({
    id: "my-custom-main",
    title: "\u0001我的专属功能",
    contexts: ["page", "selection"],
    documentUrlPatterns: ["<all_urls>"]
  });

  chrome.contextMenus.create({
    id: "my-custom-sub1",
    title: "复制当前网页链接（带备注）",
    parentId: "my-custom-main",
    contexts: ["page", "selection"],
    documentUrlPatterns: ["<all_urls>"]
  });

  chrome.contextMenus.create({
    id: "my-custom-sub2",
    title: "打开百度测试网页",
    parentId: "my-custom-main",
    contexts: ["page", "selection"],
    documentUrlPatterns: ["<all_urls>"]
  });
});

// 二、监听自定义菜单项的点击事件（原有逻辑不变）
chrome.contextMenus.onClicked.addListener((info, tab) => {
  switch (info.menuItemId) {
    case "my-custom-sub1":
      if (tab && tab.url) {
        const copyContent = `网页链接：${tab.url}（复制于 ${new Date().toLocaleString()}）`;
        try {
          navigator.clipboard.writeText(copyContent).then(() => {
            console.log(`复制成功：${copyContent}`);
          }).catch(() => {
            chrome.clipboard.writeText(copyContent);
            console.log(`复制成功（备用方案）：${copyContent}`);
          });
        } catch (err) {
          chrome.clipboard.writeText(copyContent);
          console.log(`复制成功（备用方案）：${copyContent}`);
        }
      }
      break;

    case "my-custom-sub2":
      chrome.tabs.create({
        url: "https://www.baidu.com",
        active: true
      });
      break;
  }
});

// 新增：监听内容脚本发送的双击事件消息（和内容脚本通信）
chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  // request：内容脚本发送的消息数据
  // sender：发送者信息（包含标签页、扩展ID等）
  // sendResponse：向内容脚本返回响应的函数
  if (request.type === "pageDblClick") {
    console.log("后台脚本接收到网页双击事件：", request.data);
    // 后台脚本的自定义逻辑（示例：记录双击日志、操作剪贴板等）
    const logContent = `双击日志：${new Date().toLocaleString()}，网页：${request.data.pageUrl}，目标元素：${request.data.targetTag}`;
    console.log(logContent);

    // 向内容脚本返回响应
    sendResponse({
      status: "success",
      message: "后台脚本已接收双击事件并处理完成"
    });
  }
});
