#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
//#include "easywsclient.hpp"
//#include "pulsar_peer_connection.h"
//#include "webrtc/base/win32socketserver.h"
#include "webrtc/base/json.h"
#include "webrtc/base/thread.h"
#include "pulsar_webrtc_connection.h"

//using easywsclient::WebSocket;

/* globals */
//rtc::scoped_refptr<PulsarPeerConnection> pulsar_peer_connection_;
//WebSocket::pointer ws;
std::string token;
std::string wsServerAddr;
const char *socket_path = "/tmp/mysocket";
/* /globals */

void *sock_thread(void *data)
{

    struct sockaddr_un addr;
	char buf[100];
	int fd, cl, rc;
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket err");
        exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
	unlink(socket_path);

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind err");
        exit(-1);
	}

	if (listen(fd, 5) == -1) {
        perror("listen err");
        exit(-1);
	}

	while (1) {
        if ((cl = accept(fd, NULL, NULL)) == -1) {
            perror("accept err");
            continue;
        }
        while ((rc = read(cl, buf, sizeof(buf))) > 0) {
            printf("readed\n");
            write(cl, "hihi", 5);
        }
        if (rc == -1) {
            perror("read err");
            exit(-1);
        } else if (rc == 0) {
            printf("EOF\n");
            close(cl);
        }
	}
}

int main(int argc, char *argv[])
{
    pthread_t tid;
	if (argc > 1)
	{
		wsServerAddr = std::string(argv[1]);
		std::cout << wsServerAddr << std::endl;
	}

	pthread_create(&tid, NULL, sock_thread, NULL);

	rtc::AutoThread auto_thread;
	//rtc::Thread* thread = rtc::Thread::Current();

	rtc::ThreadManager::Instance()->SetCurrentThread(&auto_thread);
	rtc::scoped_refptr<PulsarWebrtcConnection> pulsar_webrtc_connection;
	pulsar_webrtc_connection = new rtc::RefCountedObject<PulsarWebrtcConnection>();
    pulsar_webrtc_connection->CreatePeerConnection(true);
    rtc::Thread::Current()->Run();



	//std::thread wsThread(WsThread);


//	rtc::Win32Thread w32_thread;
//	rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);
//
//	pulsar_peer_connection_ = new rtc::RefCountedObject<PulsarPeerConnection>();
//	pulsar_peer_connection_->CreatePeerConnection(true);
//
//	rtc::Thread::Current()->Run();


	return 0;
}
