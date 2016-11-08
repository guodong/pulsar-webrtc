#include <iostream>
#include <thread>

#include "easywsclient.hpp"
#include "pulsar_peer_connection.h"
#include "webrtc/base/win32socketserver.h"
#include "webrtc\base\json.h"

using easywsclient::WebSocket;

/* globals */
rtc::scoped_refptr<PulsarPeerConnection> pulsar_peer_connection_;
WebSocket::pointer ws;
std::string token;
std::string wsServerAddr;
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
		pulsar_peer_connection_->peer_connection_->SetRemoteDescription(
			DummySetSessionDescriptionObserver::Create(), session_description);
		pulsar_peer_connection_->CreateAnswer();
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
		if (!pulsar_peer_connection_->peer_connection_->AddIceCandidate(candidate.get())) {
			LOG(WARNING) << "Failed to apply the received candidate";
			return;
		}
		LOG(INFO) << " Received candidate :" << message;
	}
}

void WsThread()
{
	ws = WebSocket::from_url_no_mask(wsServerAddr);
	ws->send("ready");
	while (true)
	{
		ws->poll();
		ws->dispatch(HandleWsMessage);
	}
}

int main(int argc, char *argv[])
{
	//FreeConsole();
	if (argc > 1)
	{
		wsServerAddr = std::string(argv[1]);
		std::cout << wsServerAddr << std::endl;
	}
	std::thread wsThread(WsThread);
	

	rtc::Win32Thread w32_thread;
	rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

	pulsar_peer_connection_ = new rtc::RefCountedObject<PulsarPeerConnection>();
	pulsar_peer_connection_->CreatePeerConnection(true);

	rtc::Thread::Current()->Run();
	

	return 0;
}