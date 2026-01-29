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

//每个Node的起始Index都是几千，然后最多有999个ListItem：
//后续都是用 i * LISTBOX_INDEX_STEP 表示这个ListBox中的item的起始Index的
#define LISTBOX_INDEX_START 1000
#define LISTBOX_INDEX_STEP	1000
#define ROOT_NODE_INDEX		1