========================================================================
    简易ddos攻击压测工具：ddos 项目概述
========================================================================

学习步骤:
	1. 参照 main.c 中代码写法
		
	2. 观看源码, 源码真的好简单

now window 配置步骤
	无需配置 直接使用

old window 配置步骤
	1. 配置包含目录 VC++目录
		1.1 包含目录
			$(ProjectDir)pthread/include
			$(ProjectDir)iop/include
		1.2 库目录
			$(ProjectDir)pthread/lib
			
	2. C/C++ 预处理器 -> 预处理定义
		_DEBUG
		_CRT_SECURE_NO_WARNINGS
		_CRT_NONSTDC_NO_DEPRECATE
		WIN32
		WIN32_LEAN_AND_MEAN
		_WINSOCK_DEPRECATED_NO_WARNINGS
		
	3. C/C++ 高级 -> 编译为 -> 编译为 C 代码 (/TC)

	4. 链接器 -> 附加依赖项
		pthreadVC2.lib

linux 配置步骤
	1. 参照 Makefile 文件
	
/////////////////////////////////////////////////////////////////////////////
