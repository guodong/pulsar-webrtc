#pragma once
#include "webrtc/media/base/videocapturer.h"
#include "webrtc/modules/video_capture/video_capture.h"
#include "webrtc/base/asyncinvoker.h"
#include "webrtc/modules/video_capture/video_capture_impl.h"

namespace pulsar {
	class PulsarDesktopCapturer : public cricket::VideoCapturer,
		public webrtc::VideoCaptureDataCallback
	{
	public:
		explicit PulsarDesktopCapturer();
		~PulsarDesktopCapturer();
		cricket::CaptureState Start(const cricket::VideoFormat &format);
		void Stop();
		bool IsRunning();
		bool IsScreencast() const override;
		bool GetPreferredFourccs(std::vector<uint32_t>*fourccs);
		//void SendFrame(const cricket::VideoFrame& frame, int orig_width, int orig_height);

		// Callback when a frame is captured.
		void OnIncomingCapturedFrame(const int32_t id,
			const webrtc::VideoFrame& frame) override;
		void OnCaptureDelayChanged(const int32_t id,
			const int32_t delay) override;


	private:
		rtc::scoped_refptr<webrtc::VideoCaptureModule> module_;
		rtc::Thread* start_thread_;
		std::unique_ptr<rtc::AsyncInvoker> async_invoker_;
	};
}
