#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
//#include "easywsclient.hpp"
//#include "pulsar_peer_connection.h"
//#include "webrtc/base/win32socketserver.h"
#include "webrtc/base/json.h"
#include "webrtc/base/thread.h"
#include "pulsar_webrtc_connection.h"

//using easywsclient::WebSocket;

/* globals */
//WebSocket::pointer ws;
rtc::scoped_refptr<PulsarWebrtcConnection> pulsar_webrtc_connection;
std::string token;
std::string wsServerAddr;
const char *socket_path = "/tmp/mysocket";
int fd, cl;
struct sockaddr_in clientAddr;
/* /globals */

void HandleWsMessage(const std::string &message)
{
	std::cout << message << std::endl;
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(message, jmessage))
	{
		LOG(WARNING) << "json parse error" << message;
		return;
	}

	std::string type;
	rtc::GetStringFromJsonObject(jmessage, "type", &type);
	if (!type.empty()) // sdp
	{
		std::string sdp;
		if (!rtc::GetStringFromJsonObject(jmessage, "sdp", &sdp))
		{
			LOG(WARNING) << "Can't parse received session description message.";
			return;
		}
		webrtc::SdpParseError error;
		webrtc::SessionDescriptionInterface *session_description(
			webrtc::CreateSessionDescription(type, sdp, &error)
		);
		if (!session_description) {
			LOG(WARNING) << "Can't parse received session description message. "
				<< "SdpParseError was: " << error.description;
			return;
		}
		LOG(INFO) << " Received session description :" << message;
		pulsar_webrtc_connection->peer_connection_->SetRemoteDescription(
			DummySetSessionDescriptionObserver::Create(), session_description);
		pulsar_webrtc_connection->CreateAnswer();
	}
	else
	{
		std::string sdp_mid;
		int sdp_mlineindex = 0;
		std::string sdp;
		if (!rtc::GetStringFromJsonObject(jmessage, "sdpMid",
			&sdp_mid) ||
			!rtc::GetIntFromJsonObject(jmessage, "sdpMLineIndex",
				&sdp_mlineindex) ||
			!rtc::GetStringFromJsonObject(jmessage, "sdp", &sdp)) {
			LOG(WARNING) << "Can't parse received message.";
			return;
		}
		webrtc::SdpParseError error;
		std::unique_ptr<webrtc::IceCandidateInterface> candidate(
			webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
		if (!candidate.get()) {
			LOG(WARNING) << "Can't parse received candidate message. "
				<< "SdpParseError was: " << error.description;
			return;
		}
		if (!pulsar_webrtc_connection->peer_connection_->AddIceCandidate(candidate.get())) {
			LOG(WARNING) << "Failed to apply the received candidate";
			return;
		}
		LOG(INFO) << " Received candidate :" << message;
	}
}

void *sock_thread(void *data)
{

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
	char buf[2000];
	//int rc;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket err");
        exit(-1);
	}

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind err");
        exit(-1);
	}

	/*if (listen(fd, 5) == -1) {
        perror("listen err");
        exit(-1);
	}*/
    int n = 0;
    socklen_t len = sizeof(clientAddr);
	while (1) {
        memset(buf, 0, sizeof(buf));
        /*if ((cl = accept(fd, NULL, NULL)) == -1) {
            perror("accept err");
            continue;
        }*/
        n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&clientAddr, &len);
        if (n > 0) {
            printf("%s\n", buf);
            std::string msg(buf);
            HandleWsMessage(msg);
        } else {
            perror("recv err");
        }
        /*
        while ((rc = read(cl, buf, sizeof(buf))) > 0) {
            printf("%s\n", buf);
            std::string msg(buf);
            HandleWsMessage(msg);
        }
        if (rc == -1) {
            perror("read err");
            exit(-1);
        } else if (rc == 0) {
            printf("EOF\n");
            close(cl);
        }*/
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
