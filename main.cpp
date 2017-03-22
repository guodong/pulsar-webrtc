#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "webrtc/base/json.h"
#include "webrtc/base/thread.h"
#include "pulsar_webrtc_connection.h"
#include "easywsclient.hpp"

using easywsclient::WebSocket;

/* globals */
rtc::scoped_refptr<PulsarWebrtcConnection> pulsar_webrtc_connection;
std::string token;
std::string wsServerAddr;
int fd, cl;
struct sockaddr_in clientAddr;
std::string g_sdp;
int dport;
WebSocket::pointer ws = NULL;
/* /globals */

std::string signal_addr;

void HandleWsMessage(const std::string &message)
{
	std::cout << message << std::endl;
	if (message == "ready") {
        return;
    }
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(message, jmessage)) {
		LOG(WARNING) << "json parse error" << message;
		return;
	}

	std::string type;
	rtc::GetStringFromJsonObject(jmessage, "type", &type);
	if (!type.empty()) { // sdp
		std::string sdp;
		if (!rtc::GetStringFromJsonObject(jmessage, "sdp", &sdp)) {
			LOG(WARNING) << "Can't parse received session description message.";
			return;
		}
		g_sdp = sdp;
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
	} else {
		std::string sdp_mid;
		int sdp_mlineindex = 0;

		if (!rtc::GetStringFromJsonObject(jmessage, "sdpMid",
			&sdp_mid) ||
			!rtc::GetIntFromJsonObject(jmessage, "sdpMLineIndex",
				&sdp_mlineindex) ) {
			//!rtc::GetStringFromJsonObject(jmessage, "sdp", &sdp)) {
			LOG(WARNING) << "Can't parse received message.";
			return;
		}
		webrtc::SdpParseError error;
		std::unique_ptr<webrtc::IceCandidateInterface> candidate(
			webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, g_sdp, &error));
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
    addr.sin_port = htons(dport);
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

    int n = 0;
    socklen_t len = sizeof(clientAddr);
	while (1) {
        memset(buf, 0, sizeof(buf));
        n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&clientAddr, &len);
        if (n > 0) {
            printf("%s\n", buf);
            std::string msg(buf);
            HandleWsMessage(msg);
        } else {
            perror("recv err");
        }
	}
}

void *ws_thread(void *data)
{
    ws = WebSocket::from_url_no_mask(signal_addr);

	while (ws->getReadyState() != WebSocket::CLOSED) {
        ws->poll(100);
        ws->dispatch(HandleWsMessage);
	}
	delete ws;
	return (void*)0;
}

int main(int argc, char *argv[])
{
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        std::cout << "cannot open display" << std::endl;
        exit(1);
    }
    pthread_t tid;
	if (argc > 1) {
		signal_addr = argv[1];
		std::cout << signal_addr << std::endl;
	}

	pthread_create(&tid, NULL, ws_thread, NULL);

	rtc::AutoThread auto_thread;

	rtc::ThreadManager::Instance()->SetCurrentThread(&auto_thread);
	pulsar_webrtc_connection = new rtc::RefCountedObject<PulsarWebrtcConnection>();
    pulsar_webrtc_connection->CreatePeerConnection(true);
    rtc::Thread::Current()->Run();

	return 0;
}
