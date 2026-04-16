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
//sendBookmarksToExe();
// 插件一启动就开始“等待 EXE 命令”

function startListenCommand() {
    // 👇 这就是你最熟悉的 fetch！和你发书签一模一样！
    fetch("http://127.0.0.1:8899/get-command")
    .then(res => res.text())
    .then(cmd => {
        console.log("收到 EXE 命令：", cmd);

        // 执行命令
        if (cmd === "reload-bookmarks") {
            console.log("执行：刷新书签");
            // 你写你的逻辑
        }

        // 🔥 关键：执行完立刻再次请求，持续等待！
        startListenCommand();
    })
    .catch(err => {
        console.log("EXE 未启动，5 秒后重试...");
        setTimeout(startListenCommand, 5000); // 断了自动重连
    });
}


let ws;

function startListen() {
    // 连接 EXE
    //ws = new WebSocket("ws://127.0.0.1:8899");
    ws = new WebSocket("ws://[::1]:8899");
    // 连接成功
    ws.onopen = function () {
        console.log("✅ 已连接 EXE");
    };

    // 收到命令（实时！不用轮询！）
    ws.onmessage = function (event) {
        let cmd = event.data;
        console.log("📩 收到命令：" + cmd);

        if (cmd === "reload-bookmarks") {
            console.log("🔄 执行：刷新书签");
            // 你要执行的逻辑写这里
        }
    };

    // 断开自动重连
    ws.onclose = function () {
        console.log("❌ 断开，2秒后重连");
        setTimeout(startListen, 2000);
    };

    // 出错自动重连
    ws.onerror = function () {
        console.log("======================");
        console.log("❌ 连接失败详细原因：");
        console.log("完整事件：", event);
        console.log("错误类型：", event.type);
        console.log("======================");
        ws.close();
    };
}

// 启动！
startListen();
