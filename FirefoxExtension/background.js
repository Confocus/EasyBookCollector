async function getBookmarks() {
  // debugger; // 需要调试再打开
  let bookmarksTree = await browser.bookmarks.getTree();
  let list = [];

  // 递归遍历：修复所有逻辑问题
  function traverse(node, currentPath) {
    // ------------------------------
    // 安全判断：节点无效直接跳过
    // ------------------------------
    if (!node) return;

    // ------------------------------
    // 情况 1：节点是【文件夹】（没有 url）
    // ------------------------------
    if (!node.url) {
      // 只有有标题的文件夹才加入路径（根节点无标题，不拼）
      let newPath = currentPath;
      if (node.title) {
        newPath = currentPath ? `${currentPath}/${node.title}` : node.title;
      }

      // 遍历子节点
      if (Array.isArray(node.children)) {
        node.children.forEach(child => traverse(child, newPath));
      }
    }
    // ------------------------------
    // 情况 2：节点是【书签】（有 url）
    // ------------------------------
    else if (node.url && node.title) {
      // 把【完整路径 + 标题 + URL】存入
      list.push(`[${currentPath}] ${node.title} => ${node.url}`);
    }
  }

  // ------------------------------
  // 关键修复：从根节点的【子节点】开始遍历
  // ------------------------------
  if (bookmarksTree[0]?.children) {
    bookmarksTree[0].children.forEach(rootNode => {
      traverse(rootNode, "");
    });
  }

  return list.join("\n");
}


async function sendBookmarksToExe() {
  try {
    let bookmarkText = await getBookmarks();

    // ✅ 修复：计算真实字节长度
    let encoder = new TextEncoder();
    let dataBytes = encoder.encode(bookmarkText);
    let dataLength = dataBytes.length.toString().padStart(8, '0');

    await fetch("http://127.0.0.1:8899", {
      method: "POST",
      body: dataLength
    });

    // 2. 再发内容（必须等待上一步完成）
    const res = await fetch("http://127.0.0.1:8899", {
      method: "POST",
      body: bookmarkText
    });

    const data = await res.text();
    console.log("✅ 书签发送成功", data);

  } catch (err) {
    console.error("❌ 发送失败", err);
  }
}

// 3. 执行！
sendBookmarksToExe();
