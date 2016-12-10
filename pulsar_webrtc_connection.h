#ifndef PULSAR_WEBRTC_CONNECTION_H_INCLUDED
#define PULSAR_WEBRTC_CONNECTION_H_INCLUDED

#include "webrtc/api/peerconnectioninterface.h"
#include "webrtc/api/datachannelinterface.h"
#include "interacting_event_manager.h"
#include "pulsar_desktop_capturer.h"

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
    public webrtc::CreateSessionDescriptionObserver,
    public webrtc::DataChannelObserver,
    public pulsar::EventObserver {
public:
    PulsarWebrtcConnection();
    virtual ~PulsarWebrtcConnection();

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
    bool CreatePeerConnection(bool dtls);
	void CreateAnswer();

	/* DataChannelInterface Implementation */
    void OnMessage(const webrtc::DataBuffer &buffer);
    void OnStateChange() {};

    /** pulsar::EventObserver Implementation */
    void OnEvent(const XEvent &event);

protected:
	void AddStreams();

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
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel_;
    pulsar::InteractingEventManager *iem;
    pulsar::PulsarDesktopCapturer *desktop_capturer;
    Display *display;
    Window root;
};

#endif // PULSAR_WEBRTC_CONNECTION_H_INCLUDED
