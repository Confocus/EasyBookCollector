// content.js：注入到网页中的内容脚本，监听双击事件
console.log("内容脚本已注入，开始监听网页双击事件...");

// 监听网页的双击事件（dblclick 是原生 DOM 事件，对应双击操作）
document.addEventListener('dblclick', (e) => {
  // e：双击事件对象，包含双击位置、目标元素等信息
  console.log("检测到网页双击事件！");
  console.log("双击位置：x=" + e.clientX + ", y=" + e.clientY);
  console.log("双击目标元素：", e.target);
  // 1. 获取当前页面的选中对象
  const selection = window.getSelection();

  // 2. 核心判断逻辑：区分双击选中文本 vs 双击空白
  // 条件1：选中的文本内容是否为空（去除首尾空格后）
  const hasSelectedText = selection.toString().trim() !== '';
  // 条件2：选中范围是否有效（避免选中"空范围"的特殊情况）
  const hasValidRange = !selection.isCollapsed;

  // 3. 分情况处理
  if (hasSelectedText && hasValidRange) {
    // 双击选中了文字
    //console.log('✅ 双击选中了文字，选中内容：', selection.toString().trim());
    //alert(`选中的文字：${selection.toString().trim()}`);
  } else {
    // 双击了空白位置（无文字选中）
    console.log('❌ 双击了空白位置，未选中任何文字');
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
  }
  // 你的自定义逻辑：双击后执行的操作（示例：弹出提示、修改网页内容、和后台脚本通信等）



});
