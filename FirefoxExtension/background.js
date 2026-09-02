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

async function sendBookmarksToExe() {
    console.log("sendBookmarksToExe start");

    try {
        // 获取书签
        let bookmarkText = await getBookmarks();
        console.log("✅ 准备发送书签数据，长度：", bookmarkText.length);

        // --- WebSocket 直接发送！一行搞定！---
        ws.send(bookmarkText);

        console.log("✅ 书签已发送到 EXE！");

    } catch (err) {
        console.error("❌ 发送失败：", err);
    }
}

async function getBookmarks2() {
  // debugger; // 需要调试再打开
  let bookmarksTree = await browser.bookmarks.getTree();
  let list = [];

  /**
   * @param {object} node 当前节点
   * @param {string} currentPath 当前路径字符串
   * @param {string|null} parentFolderId 当前节点所属父文件夹id
   */
  function traverse(node, currentPath, parentFolderId) {
    if (!node) return;

    // 文件夹节点：node.id = 当前这个文件夹自己的id
    if (!node.url) {
      let newPath = currentPath;
      if (node.title) {
        newPath = currentPath ? `${currentPath}/${node.title}` : node.title;
      }
      // 子节点的父文件夹id就是本文件夹的 node.id
      if (Array.isArray(node.children)) {
        node.children.forEach(child => traverse(child, newPath, node.id));
      }
    }
    // 书签节点：记录它所在文件夹 parentFolderId，以及书签自身id node.id
    else if (node.url && node.title) {
      list.push(`[${currentPath}] folderId:${parentFolderId} | bookmarkId:${node.id} | ${node.title} => ${node.url}`);
    }
  }

  if (bookmarksTree[0]?.children) {
    bookmarksTree[0].children.forEach(rootNode => {
      // 顶层三个文件夹（工具栏/书签菜单/其他书签），它们的parentId是系统根，这里传入rootNode.id作为子节点的父ID
      traverse(rootNode, "", null);
    });
  }

  return list.join("\n");
}

async function sendBookmarksToExe2() {
    console.log("sendBookmarksToExe2 start");

    try {
        // 获取书签
        let bookmarkText = await getBookmarks2();
        console.log("✅ 准备发送书签数据，长度：", bookmarkText.length);

        // --- WebSocket 直接发送！一行搞定！---
        ws.send(bookmarkText);

        console.log("✅ 书签已发送到 EXE！");

    } catch (err) {
        console.error("❌ 发送失败：", err);
    }
}

async function getActiveTabInfo() {
    const tabData = await getSavedBlurTab();
            // 转为JSON字符串发送
            ws.send(JSON.stringify({
                type: "before_switch_tab",
                data: tabData
            }));
}

/**
 * 根据父文件夹id插入书签
 * @param {string} parentId 目标文件夹ID
 * @param {string} title 书签标题
 * @param {string} url 书签地址
 * @returns {Promise<object>} 返回新建的书签节点对象，包含新建书签的id
 */
async function addBookmarkByParentId(parentId, title, url) {
  try {
    const bookmarkNode = await browser.bookmarks.create({
      parentId: parentId,
      title: title,
      url: url
    });
    console.log("插入成功，新书签id：", bookmarkNode.id);
    return bookmarkNode;
  } catch (err) {
    console.error("插入书签失败", err);
    throw err;
  }
}

// async function getRecordTab() {
//     return await browser.runtime.sendMessage({
//         action: "getBeforeBlurTab"
//     });
// }

// async function sendActiveTabInfoToExe() {
//     console.log("sendActiveTabInfoToExe start");

//     try {
//         // 获取书签
//         let tabInfo = await getActiveTabInfo();
//         console.log("✅ 准备发送tabInfo数据，长度：", tabInfo.length);

//         // --- WebSocket 直接发送！一行搞定！---
//         ws.send(tabInfo);

//         console.log("✅ tabInfo已发送到 EXE！");

//     } catch (err) {
//         console.error("❌ tabInfo发送失败：", err);
//     }
// }

let ws;

function startListen() {
    // 连接 EXE
    //ws = new WebSocket("ws://127.0.0.1:8899");
    console.log("retry at", new Date().toLocaleTimeString());
    ws = new WebSocket("ws://[::1]:8899");
    // 连接成功
    ws.onopen = function () {
        console.log("✅ 已连接 EXE");
    };

    // 收到命令（实时！不用轮询！）
    ws.onmessage = function (event) {
        let cmd = event.data;
        console.log("📩 收到命令：" + cmd);

        if (cmd === "reload-bookmarks") 
        {
            console.log("🔄 执行：刷新书签");
            // 你要执行的逻辑写这里
            sendBookmarksToExe();
        }
        if (cmd === "reload-bookmarks-id") 
        {
            console.log("🔄 执行：刷新书签");
            // 你要执行的逻辑写这里
            sendBookmarksToExe2();
        }
        if(cmd == "AddActiveTab")
        {
            console.log("🔄 获取activeTab信息");
            getActiveTabInfo();
        }
        if(cmd == "add-bookmark")
        {
            console.log("🔄 首次尝试同步本地书签到远端");
            addBookmarkByParentId("JUbKkSQG6fFs", "aaa", "https://example.com");
        }
        if (cmd === "retry") {
            console.log("🔄 retry");
        }
    };

    // 断开自动重连
    ws.onclose = function () {
        //console.log("❌ 断开，2秒后重连");
        //setTimeout(startListen, 2000);
    };

    // 出错自动重连
    ws.onerror = function (event) {
        console.log("======================");
        console.log("❌ 连接失败详细原因：");
        console.log("完整事件：", event);
        console.log("错误类型：", event.type);
        console.log("======================");
        ws.close();
    };
}

initTabBlurRecorder();
// 启动！
startListen();
