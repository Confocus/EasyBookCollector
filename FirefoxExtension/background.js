// 创建右键菜单项 —— 仅在“标签页”上出现
browser.menus.create({
    id: "save-bookmark",
    title: "保存此书",
    contexts: ["tab"]
});

const manifest = browser.runtime.getManifest();
const supportedUrls = manifest.content_scripts[0].matches.map(pattern =>{
    // 去掉最后的 * 号，方便 startsWith 判断
    return pattern.replace(/\*$/, "");
});

// 处理点击事件
browser.menus.onClicked.addListener((info, tab) =>{
    if (info.menuItemId === "save-bookmark") {
        const url = tab.url;
        const isSupported = supportedUrls.some(prefix =>url.startsWith(prefix));
        console.log("你点击了 XXX，标签页：", tab);
        if (isSupported) {
            browser.tabs.sendMessage ?. (tab.id, {
                action: "save-bookmark"
            }).
            catch(err =>{
              console.error("isSupported失败：", err);
});
            console.log("准备执行browser.runtime.sendNativeMessage：", tab);
            browser.runtime.sendNativeMessage("com.demo.native_host", // Host 名称
            { action: "test" }).then(response =>{
                console.log("Native 返回：", response);
            }).
            catch(err =>{
                console.error("Native 连接失败：", err);
            });

        } else {
            browser.notifications.create({
                type: "basic",
                //iconUrl: browser.runtime.getURL("icons/border-48.png"),
                title: "提示",
                message: "当前页面未被插件支持！"
            });
        }

    }
});