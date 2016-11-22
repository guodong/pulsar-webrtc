#ifndef PULSAR_WEBRTC_CONNECTION_H_INCLUDED
#define PULSAR_WEBRTC_CONNECTION_H_INCLUDED

#include "webrtc/api/peerconnectioninterface.h"

class DummySetSessionDescriptionObserver
	: public webrtc::SetSessionDescriptionObserver {
public:
	static DummySetSessionDescriptionObserver* Create() {
		return
			new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() {
		LOG(INFO) << __FUNCTION__;
	}
	virtual void OnFailure(const std::string& error) {
		LOG(INFO) << __FUNCTION__ << " " << error;
	}
protected:
	DummySetSessionDescriptionObserver() {}
	~DummySetSessionDescriptionObserver() {}
};

class PulsarWebrtcConnection : public webrtc::PeerConnectionObserver,
    public webrtc::CreateSessionDescriptionObserver {
public:
    PulsarWebrtcConnection();
    virtual ~PulsarWebrtcConnection();

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
    bool CreatePeerConnection(bool dtls);
	void CreateAnswer();

protected:
	void AddStreams();
	cricket::VideoCapturer* OpenVideoCaptureDevice();

	void OnSignalingChange(
		webrtc::PeerConnectionInterface::SignalingState new_state) override {};
	void OnAddStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {};
	void OnRemoveStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {};
	void OnDataChannel(
		rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;
	void OnRenegotiationNeeded() override {}
	void OnIceConnectionChange(
		webrtc::PeerConnectionInterface::IceConnectionState new_state) override {};
	void OnIceGatheringChange(
		webrtc::PeerConnectionInterface::IceGatheringState new_state) override {};
	void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
	void OnIceConnectionReceivingChange(bool receiving) override {}

	// CreateSessionDescriptionObserver implementation.
	virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
	virtual void OnFailure(const std::string& error);

private:
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
		peer_connection_factory_;
};

#endif // PULSAR_WEBRTC_CONNECTION_H_INCLUDED
