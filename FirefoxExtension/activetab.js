// tabBlurRecorder.js 【V2专用，去掉export】
function initTabBlurRecorder() {
    // 常量：Firefox 全局失焦标记
    const WINDOW_ID_NONE = browser.windows.WINDOW_ID_NONE;
    // 内存缓存：持续刷新当前浏览器活跃标签
    let lastActiveTab = null;
    // 防抖定时器，过滤浏览器内部窗口切换产生的临时-1事件
    let focusDebounceTimer = null;

    /**
     * 刷新浏览器当前活跃标签（最后聚焦窗口的激活Tab）
     */
    async function refreshActiveTabCache() {
        try {
            const tabs = await browser.tabs.query({
                active: true,
                lastFocusedWindow: true
            });
            if (tabs.length > 0) {
                // 深拷贝防止对象引用被后续覆盖
                lastActiveTab = structuredClone(tabs[0]);
            }
        } catch (err) {
            console.error("刷新活跃标签缓存失败：", err);
        }
    }

    /**
     * 保存【切换到外部EXE之前】的页面快照到持久化存储
     */
    async function saveBeforeBlurSnapshot() {
        if (!lastActiveTab) return;
        // 兼容低版本Firefox：如果不支持storage.session，只写入local
        const saveTasks = [];
        if (browser.storage.session) {
            saveTasks.push(browser.storage.session.set({ beforeBlurTab: lastActiveTab }));
        }
        saveTasks.push(browser.storage.local.set({ lastBrowserTabBeforeAppSwitch: lastActiveTab }));
        await Promise.all(saveTasks);
        console.log("已记录切出浏览器前页面：", lastActiveTab.title, lastActiveTab.url);
    }

    /**
     * 对外工具函数：随时获取「切换到其他EXE之前记录的浏览器标签」
     */
    async function getTabBeforeSwitchOtherExe() {
        // 优先读取临时会话存储，速度更快
        if (browser.storage.session) {
            const sessionData = await browser.storage.session.get("beforeBlurTab");
            if (sessionData.beforeBlurTab) {
                return sessionData.beforeBlurTab;
            }
        }
        // 兜底读取持久化存储
        const localData = await browser.storage.local.get("lastBrowserTabBeforeAppSwitch");
        return localData.lastBrowserTabBeforeAppSwitch ?? null;
    }

    // 将读取函数挂载到全局，给background.js调用
    window.getSavedBlurTab = getTabBeforeSwitchOtherExe;

    // 初始化缓存
    refreshActiveTabCache();

    // 监听1：标签切换（Alt+Tab切换浏览器Tab），更新缓存
    browser.tabs.onActivated.addListener(() => {
        clearTimeout(focusDebounceTimer);
        refreshActiveTabCache();
    });

    // 监听2：浏览器窗口焦点变化（核心）
    browser.windows.onFocusChanged.addListener((windowId) => {
        // 清空防抖计时器，过滤高频事件
        clearTimeout(focusDebounceTimer);

        if (windowId === WINDOW_ID_NONE) {
            // 收到全局失焦，延迟200ms判断：是切外部程序，不是浏览器内部窗口切换
            focusDebounceTimer = setTimeout(async () => {
                // 二次校验：当前依然是无浏览器窗口焦点，确认是切到其他EXE
                const lastWin = await browser.windows.getLastFocused();
                if (!lastWin.focused) {
                    await saveBeforeBlurSnapshot();
                }
            }, 200);
        } else {
            // 浏览器窗口重新获得焦点，刷新最新活跃标签
            refreshActiveTabCache();
        }
    });

    // 暴露给popup/内容脚本调用消息监听
    browser.runtime.onMessage.addListener(async (msg) => {
        if (msg.action === "getBeforeBlurTab") {
            return await getTabBeforeSwitchOtherExe();
        }
    });
}

// 全局初始化函数，background直接调用
window.initTabBlurRecorder = initTabBlurRecorder;
