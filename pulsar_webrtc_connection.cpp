#include <iostream>
#include <netinet/in.h>

#include "pulsar_webrtc_connection.h"
#include "pulsar_desktop_capturer.h"
#include "interacting_event_manager.h"
#include "webrtc/api/peerconnectioninterface.h"
#include "webrtc/api/test/fakeconstraints.h"
#include "webrtc/base/json.h"
#include "webrtc/media/engine/webrtcvideocapturerfactory.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"
#include "webrtc/modules/desktop_capture/desktop_capturer.h"
#include "webrtc/media/base/videocapturer.h"
#include <X11/extensions/XTest.h>

using pulsar::PulsarDesktopCapturer;

extern int fd;
extern struct sockaddr_in clientAddr;

void PulsarWebrtcConnection::OnMessage(const webrtc::DataBuffer &bf)
{
    std::unique_ptr<webrtc::DataBuffer> buffer(new webrtc::DataBuffer(bf));
    printf("datachannel: %s\n", buffer->data.data());
    std::string json_string(buffer->data.data<char>());
    Json::Reader reader;
    Json::Value jmessage;
    if (!reader.parse(json_string, jmessage))
    {
        LOG(WARNING) << "json parse error" << buffer->data.data();
        return;
    }
    std::string msg;
    rtc::GetStringFromJsonObject(jmessage, "msg", &msg);
    Json::Value payload;
    rtc::GetValueFromJsonObject(jmessage, "payload", &payload);
    if (!msg.compare("mousemove"))
    {
        printf("move\n");
        unsigned int x, y;
        rtc::GetUIntFromJsonObject(payload, "x", &x);
        rtc::GetUIntFromJsonObject(payload, "y", &y);
        printf("%d, %d\n", x, y);
        XWarpPointer(display, None, root, 0, 0, 0, 0, x, y);
        XFlush(display);
    }
    else if (!msg.compare("mousedown"))
    {
        printf("mousedown\n");
        unsigned int code;
        rtc::GetUIntFromJsonObject(payload, "code", &code);
        XTestFakeButtonEvent(display, code, True, CurrentTime);
        XFlush(display);
    }
    else if (!msg.compare("mouseup"))
    {
        printf("mouseup\n");
        unsigned int code;
        rtc::GetUIntFromJsonObject(payload, "code", &code);
        XTestFakeButtonEvent(display, code, False, CurrentTime);
        XFlush(display);
    }
    else if (!msg.compare("keydown"))
    {
        unsigned int code;
        rtc::GetUIntFromJsonObject(payload, "code", &code);
        char c[2] = {code, 0};
        KeySym ks = XStringToKeysym(c);
        KeyCode kc = XKeysymToKeycode(display, ks);
        XTestFakeKeyEvent(display, kc, True, CurrentTime);
        XFlush(display);
    }
    else if (!msg.compare("keyup"))
    {
        unsigned int code;
        rtc::GetUIntFromJsonObject(payload, "code", &code);
        char c[2] = {code, 0};
        KeySym ks = XStringToKeysym(c);
        KeyCode kc = XKeysymToKeycode(display, ks);
        XTestFakeKeyEvent(display, kc, False, CurrentTime);
        XFlush(display);
    }
}

PulsarWebrtcConnection::PulsarWebrtcConnection()
{
    display = XOpenDisplay(NULL);

    root = RootWindow(display, DefaultScreen(display));
}

PulsarWebrtcConnection::~PulsarWebrtcConnection()
{
    XCloseDisplay(display);
}

void PulsarWebrtcConnection::OnEvent(const XEvent &event)
{
    printf("get event: %d\n", event.type);
    switch (event.type)
    {
    case CreateNotify: {

        Json::StyledWriter writer;
        Json::Value jmessage;
        Json::Value payload;
        payload["id"] = (int)event.xcreatewindow.window;
        payload["x"] = event.xcreatewindow.x;
        payload["y"] = event.xcreatewindow.y;
        payload["width"] = event.xcreatewindow.width;
        payload["height"] = event.xcreatewindow.height;
        payload["bare"] = event.xcreatewindow.override_redirect ? 1 : 0;
        jmessage["resource"] = "window";
        jmessage["action"] = "create";
        jmessage["payload"] = payload;
        std::string msg(writer.write(jmessage));
        webrtc::DataBuffer db(msg);
        data_channel_->Send(db);
        break;
    }
    case MapNotify: {
        Json::StyledWriter writer;
        Json::Value jmessage;
        Json::Value payload;
        payload["id"] = (int)event.xmap.window;
        jmessage["resource"] = "window";
        jmessage["action"] = "show";
        jmessage["payload"] = payload;
        std::string msg(writer.write(jmessage));
        webrtc::DataBuffer db(msg);
        data_channel_->Send(db);
        break;
    }
    case UnmapNotify: {
        Json::StyledWriter writer;
        Json::Value jmessage;
        Json::Value payload;
        payload["id"] = (int)event.xunmap.window;
        jmessage["resource"] = "window";
        jmessage["action"] = "hide";
        jmessage["payload"] = payload;
        std::string msg(writer.write(jmessage));
        webrtc::DataBuffer db(msg);
        data_channel_->Send(db);
        break;
    }
    case ConfigureNotify: {
        Json::StyledWriter writer;
        Json::Value jmessage;
        Json::Value payload;
        payload["id"] = (int)event.xconfigure.window;
        payload["x"] = event.xconfigure.x;
        payload["y"] = event.xconfigure.y;
        payload["width"] = event.xconfigure.width;
        payload["height"] = event.xconfigure.height;
        payload["bare"] = event.xconfigure.override_redirect ? 1 : 0;
        jmessage["resource"] = "window";
        jmessage["action"] = "configure";
        jmessage["payload"] = payload;
        std::string msg(writer.write(jmessage));
        webrtc::DataBuffer db(msg);
        data_channel_->Send(db);
        break;
    }
    case DestroyNotify: {
        Json::StyledWriter writer;
        Json::Value jmessage;
        Json::Value payload;
        payload["id"] = (int)event.xdestroywindow.window;
        jmessage["resource"] = "window";
        jmessage["action"] = "destroy";
        jmessage["payload"] = payload;
        std::string msg(writer.write(jmessage));
        webrtc::DataBuffer db(msg);
        data_channel_->Send(db);
        break;
    }
    default:
        if (event.type == iem->GetDamageEventBase() + XDamageNotify)
        {

            printf("%s\n", "get damage");
            XDamageSubtract(iem->GetDisplay(), iem->GetDamageHandle(), None, None);
            desktop_capturer->CaptureFrame();
            desktop_capturer->CaptureFrame();
            desktop_capturer->CaptureFrame();
            desktop_capturer->CaptureFrame();
        }
        break;
    }
}

void PulsarWebrtcConnection::AddStreams()
{
    desktop_capturer = new PulsarDesktopCapturer();
    //desktop_capturer->Start();
    webrtc::FakeConstraints video_constraints;
    video_constraints.AddMandatory(
        webrtc::MediaConstraintsInterface::kMinFrameRate, 30);
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
        peer_connection_factory_->CreateVideoTrack(
            "video_label",
            peer_connection_factory_->CreateVideoSource(desktop_capturer,
                    &video_constraints)));
    rtc::scoped_refptr<webrtc::MediaStreamInterface> stream =
        peer_connection_factory_->CreateLocalMediaStream("stream_label");
    stream->AddTrack(video_track);
    if (!peer_connection_->AddStream(stream))
    {
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
    if (dtls)
    {
        constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
                                "true");
    }
    else
    {
        constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
                                "false");
    }
    peer_connection_ = peer_connection_factory_->CreatePeerConnection(
                           config, &constraints, NULL, NULL, this);
    if (peer_connection_.get() == NULL)
    {
        return false;
    }
    AddStreams();


    return true;
}

void PulsarWebrtcConnection::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    std::cout << "hehe" << std::endl;
    data_channel_ = channel;
    //PulsarDataChannelObserver *ob = new PulsarDataChannelObserver();
    data_channel_->RegisterObserver(this);

    /** listen interacting events **/
    iem = new pulsar::InteractingEventManager();
    iem->RegisterObserver(this);
    //XInitThreads();
    iem->Start();
}

void PulsarWebrtcConnection::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    Json::StyledWriter writer;
    Json::Value jmessage;
    jmessage["sdpMid"] = candidate->sdp_mid();
    jmessage["sdpMLineIndex"] = candidate->sdp_mline_index();
    std::string sdp;
    if (!candidate->ToString(&sdp))
    {
        LOG(LS_ERROR) << "Failed to serialize candidate";
        return;
    }
    jmessage["candidate"] = sdp;
    std::string msg = writer.write(jmessage);
    std::cout << msg;
    //ws->send(msg);
    //write(cl, msg.c_str(), msg.length());
    sendto(fd, msg.c_str(), msg.length(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    sleep(1);
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
    std::string msg = writer.write(jmessage);
    std::cout << msg;
    //ws->send(writer.write(jmessage));
    //write(cl, msg.c_str(), msg.length());
    sendto(fd, msg.c_str(), msg.length(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    sleep(1);
}

void PulsarWebrtcConnection::OnFailure(const std::string& error)
{

}
