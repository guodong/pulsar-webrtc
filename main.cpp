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
	ws = WebSocket::from_url_no_mask("ws://106.75.71.14:3000");
	while (true)
	{
		ws->poll();
		ws->dispatch(HandleWsMessage);
	}
}

int main(int argc, char *argv[])
{
	/*PeerConnectionClient client;
	rtc::scoped_refptr<Conductor> conductor(
		new rtc::RefCountedObject<Conductor>(&client)
	);*/

	std::thread wsThread(WsThread);

	//wsThread.join();
	

	rtc::Win32Thread w32_thread;
	rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

	std::string token(argv[1]);
	PulsarPeerConnection *p = new rtc::RefCountedObject<PulsarPeerConnection>();
	//rtc::scoped_refptr<PulsarPeerConnection> p(new rtc::RefCountedObject<PulsarPeerConnection>());
	pulsar_peer_connection_ = p;
	pulsar_peer_connection_->CreatePeerConnection(true);

	/*Signaling signaling(token);
	signaling.server_address.SetIP("106.75.71.14");
	signaling.server_address.SetPort(3000);
	signaling.GetPeerInfo();*/

	rtc::Thread::Current()->Run();
	

	return 0;
}