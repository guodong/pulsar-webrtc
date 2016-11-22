#include <iostream>

#include "webrtc/api/peerconnectioninterface.h"
#include "webrtc/api/test/fakeconstraints.h"
#include "webrtc/base/json.h"
#include "webrtc/media/engine/webrtcvideocapturerfactory.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"
#include "webrtc/modules/desktop_capture/desktop_capturer.h"
#include "webrtc/media/base/videocapturer.h"
#include "pulsar_webrtc_connection.h"
#include "pulsar_desktop_capturer.h"

using pulsar::PulsarDesktopCapturer;

PulsarWebrtcConnection::PulsarWebrtcConnection()
{
}

PulsarWebrtcConnection::~PulsarWebrtcConnection()
{
}

cricket::VideoCapturer* PulsarWebrtcConnection::OpenVideoCaptureDevice() {
	std::vector<std::string> device_names;
	{
		std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
			webrtc::VideoCaptureFactory::CreateDeviceInfo(0));
		if (!info) {
			return nullptr;
		}
		int num_devices = info->NumberOfDevices();
		for (int i = 0; i < num_devices; ++i) {
			const uint32_t kSize = 256;
			char name[kSize] = { 0 };
			char id[kSize] = { 0 };
			if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
				device_names.push_back(name);
			}
		}
	}
	cricket::WebRtcVideoDeviceCapturerFactory factory;
	cricket::VideoCapturer* capturer = nullptr;
	for (const auto& name : device_names) {
		capturer = factory.Create(cricket::Device(name, 0));
		if (capturer) {
			break;
		}
	}
	return capturer;
}

void PulsarWebrtcConnection::AddStreams() {

	webrtc::FakeConstraints video_constraints;
	video_constraints.AddMandatory(
		webrtc::MediaConstraintsInterface::kMinFrameRate, 30);
	rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
		peer_connection_factory_->CreateVideoTrack(
			"video_label",
			peer_connection_factory_->CreateVideoSource(new PulsarDesktopCapturer(),
				&video_constraints)));
	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream =
		peer_connection_factory_->CreateLocalMediaStream("stream_label");
	stream->AddTrack(video_track);
	if (!peer_connection_->AddStream(stream)) {
		LOG(LS_ERROR) << "Adding stream to PeerConnection failed";
	}
}

bool PulsarWebrtcConnection::CreatePeerConnection(bool dtls)
{
	peer_connection_factory_ = webrtc::CreatePeerConnectionFactory();
	webrtc::PeerConnectionInterface::RTCConfiguration config;
	webrtc::PeerConnectionInterface::IceServer server;
	server.uri = "turn:106.75.71.14:3478?transport=udp";
	server.username = "gd";
	server.password = "gd";
	config.servers.push_back(server);
	config.disable_ipv6 = true;
	webrtc::FakeConstraints constraints;
	if (dtls) {
		constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
			"true");
	}
	else {
		constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
			"false");
	}
	peer_connection_ = peer_connection_factory_->CreatePeerConnection(
		config, &constraints, NULL, NULL, this);
	if (peer_connection_.get() == NULL) {
		return false;
	}
	AddStreams();


	return true;
}

void PulsarWebrtcConnection::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
	std::cout << "hehe" << std::endl;

}

void PulsarWebrtcConnection::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
	Json::StyledWriter writer;
	Json::Value jmessage;
	jmessage["sdpMid"] = candidate->sdp_mid();
	jmessage["sdpMLineIndex"] = candidate->sdp_mline_index();
	std::string sdp;
	if (!candidate->ToString(&sdp)) {
		LOG(LS_ERROR) << "Failed to serialize candidate";
		return;
	}
	jmessage["candidate"] = sdp;
	std::string msg = writer.write(jmessage);
	std::cout << msg;
	//ws->send(msg);
}

void PulsarWebrtcConnection::CreateAnswer()
{
	peer_connection_->CreateAnswer(this, NULL);
}

void PulsarWebrtcConnection::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
	peer_connection_->SetLocalDescription(
		DummySetSessionDescriptionObserver::Create(), desc);

	std::string sdp;
	desc->ToString(&sdp);

	Json::StyledWriter writer;
	Json::Value jmessage;
	jmessage["type"] = desc->type();
	jmessage["sdp"] = sdp;
	//ws->send(writer.write(jmessage));
}

void PulsarWebrtcConnection::OnFailure(const std::string& error)
{

}
