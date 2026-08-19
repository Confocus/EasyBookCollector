// header.h: 标准系统包含文件的包含文件，
// 或特定于项目的包含文件
//
#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <Windows.h>
#include <iostream>

//每个Node的起始Index都是几千，然后最多有999个ListItem：
//后续都是用 i * LISTBOX_INDEX_STEP 表示这个ListBox中的item的起始Index的
#define LISTBOX_INDEX_START 1000
#define LISTBOX_INDEX_STEP	1000
#define ROOT_NODE_INDEX		1


#define ID_POPUP_DELETE 4001
#define ID_POPUP_ADD 4002

#define STRING_RELOAD_BOOKMARKS	"reload-bookmarks"
#define UID_RELOAD_BOOKMARKS	1
#define STRING_ADD_ACTIVE_TAB	"AddActiveTab"
#define UID_ADD_ACTIVE_TAB		2
#define STRING_DISCONNECT "DisconnectPipe"
#define UID_DISCONNECT		3

#define PIPE_NAME_BOOKMARK_TRANS	L"\\\\.\\pipe\\BookmarkTransPipe"
#define EVENT_NAME_SENT_RECV_CMD	L"{31E3A6F1-105A-45D9-8E73-79CE24064F5C}\SendRecvCmd"
#define EVENT_NAME_RESPONSE			L"{A7486818-B995-4F67-BA45-834BE0B980EC}\Response"
#define EVENT_NAME_CONNECT_PIPE		L"{A1418B8A-7998-4262-9D44-47E607653E93}\ConnectPipe"
#define EVENT_NAME_DISCONNECT_PIPE	L"{4E17318B-F76A-448B-8401-42085E3AC90D}\DisconnectPipe"
#define EVENT_NAME_CMD_FINISHED		L"{08D7B0CC-08CA-4823-AE7F-55585EC28A5B}\CommandFinished"
#define EVENT_NAME_LOADED_BOOKMARKS	L"{08D7B0CC-08CA-4823-AE7F-55585EC28A5B}\LoadedBookmarks"
