#pragma once
//todo：完善添加某个书签的功能，要求同步到远端的目录下；这就进一步要求本地要保存bookmarkid和parentid

//todo：未来拿到评分数据
//todo：添加一个文件夹后如何同步到远端？
//todo：进入目录后添加如何刷新添加后的页面？
//todo：未来要改成浏览网页时就在本地存储好，而不是再去进行网络请求？
//todo：实现彻底的添加和删除功能？
//todo：当VS以管理员权限启动的时候，管道通信会失败？
//todo：后期这里要改成队列m_InsertedFolder
//todo：新增回收站服务，浏览器上的标签都是一点删除就无法恢复的
//todo：bugfix,空文件夹是不会显示出来的。经测试，空文件夹并不增加vector中节点的数量
//todo：尽早引入gtest
//todo：新建文件夹功能
//todo：验证js的asyn是否正确