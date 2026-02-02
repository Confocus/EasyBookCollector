// content.js：注入到网页中的内容脚本，监听双击事件
console.log("内容脚本已注入，开始监听网页双击事件...");

// 监听网页的双击事件（dblclick 是原生 DOM 事件，对应双击操作）
document.addEventListener('dblclick', (e) => {
  // e：双击事件对象，包含双击位置、目标元素等信息
  console.log("检测到网页双击事件！");
  console.log("双击位置：x=" + e.clientX + ", y=" + e.clientY);
  console.log("双击目标元素：", e.target);

  // 你的自定义逻辑：双击后执行的操作（示例：弹出提示、修改网页内容、和后台脚本通信等）
  alert("检测到网页双击！你双击了：" + e.target.tagName);

  // 可选：如果需要和 background.js 通信（比如执行扩展的全局功能）
  chrome.runtime.sendMessage({
    type: "pageDblClick",
    data: {
      targetTag: e.target.tagName,
      pageUrl: window.location.href
    }
  }, (response) => {
    console.log("后台脚本返回的响应：", response);
  });
});
