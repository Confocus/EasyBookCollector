// 一、扩展安装/更新时，创建自定义右键菜单项（仅执行一次）
chrome.runtime.onInstalled.addListener(() => {
  // 1. 创建一级菜单
  chrome.contextMenus.create({
    id: "my-custom-main",
    title: "测试我的专属功能",
    contexts: ["page", "selection"]
  });

  // 2. 创建子菜单 1：复制当前网页链接（带备注）
  chrome.contextMenus.create({
    id: "my-custom-sub1",
    title: "复制当前网页链接（带备注）",
    parentId: "my-custom-main",
    contexts: ["page", "selection"]
  });

  // 3. 创建子菜单 2：打开测试网页
  chrome.contextMenus.create({
    id: "my-custom-sub2",
    title: "打开百度测试网页",
    parentId: "my-custom-main",
    contexts: ["page", "selection"]
  });
});

// 二、监听自定义菜单项的点击事件，执行对应功能
chrome.contextMenus.onClicked.addListener((info, tab) => {
  switch (info.menuItemId) {
    case "my-custom-sub1":
      // 功能 1：复制当前网页链接 + 备注（修改点：适配 V2，用更稳定的剪贴板逻辑）
      if (tab && tab.url) {
        const copyContent = `网页链接：${tab.url}（复制于 ${new Date().toLocaleString()}）`;
        
        // 微调：Manifest V2 中，部分浏览器不支持 navigator.clipboard，补充兼容逻辑
        try {
          // 优先使用 navigator.clipboard
          navigator.clipboard.writeText(copyContent).then(() => {
            console.log(`复制成功：${copyContent}`);
          }).catch(() => {
            // 备用方案：如果上面失败，使用扩展专属 API（需要 clipboardWrite 权限）
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
      // 功能 2：新建标签页，打开百度测试网页（逻辑不变）
      chrome.tabs.create({
        url: "https://www.baidu.com",
        active: true
      });
      break;
  }
});