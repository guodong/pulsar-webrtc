#include <thread>
#include <iostream>
#include <cmath>

#include "pulsar_desktop_capturer.h"
#include "webrtc\media\base\videocommon.h"
#include "webrtc\base\arraysize.h"
#include "webrtc\modules\desktop_capture\desktop_capturer.h"
#include "webrtc\modules\desktop_capture\screen_capturer.h"
#include "webrtc\modules\desktop_capture\desktop_capture_options.h"
#include "webrtc\modules\desktop_capture\desktop_and_cursor_composer.h"

#define FOURCC(a, b, c, d)                                        \
  ((static_cast<uint32_t>(a)) | (static_cast<uint32_t>(b) << 8) | \
   (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24))
// Some pages discussing FourCC codes:
//   http://www.fourcc.org/yuv.php
//   http://v4l2spec.bytesex.org/spec/book1.htm
//   http://developer.apple.com/quicktime/icefloe/dispatch020.html
//   http://msdn.microsoft.com/library/windows/desktop/dd206750.aspx#nv12
//   http://people.xiph.org/~xiphmont/containers/nut/nut4cc.txt

// FourCC codes grouped according to implementation efficiency.
// Primary formats should convert in 1 efficient step.
// Secondary formats are converted in 2 steps.
// Auxilliary formats call primary converters.
enum FourCC {
	// 9 Primary YUV formats: 5 planar, 2 biplanar, 2 packed.
	FOURCC_I420 = FOURCC('I', '4', '2', '0'),
	FOURCC_I422 = FOURCC('I', '4', '2', '2'),
	FOURCC_I444 = FOURCC('I', '4', '4', '4'),
	FOURCC_I411 = FOURCC('I', '4', '1', '1'),
	FOURCC_I400 = FOURCC('I', '4', '0', '0'),
	FOURCC_NV21 = FOURCC('N', 'V', '2', '1'),
	FOURCC_NV12 = FOURCC('N', 'V', '1', '2'),
	FOURCC_YUY2 = FOURCC('Y', 'U', 'Y', '2'),
	FOURCC_UYVY = FOURCC('U', 'Y', 'V', 'Y'),

	// 2 Secondary YUV formats: row biplanar.
	FOURCC_M420 = FOURCC('M', '4', '2', '0'),

	// 9 Primary RGB formats: 4 32 bpp, 2 24 bpp, 3 16 bpp.
	FOURCC_ARGB = FOURCC('A', 'R', 'G', 'B'),
	FOURCC_BGRA = FOURCC('B', 'G', 'R', 'A'),
	FOURCC_ABGR = FOURCC('A', 'B', 'G', 'R'),
	FOURCC_24BG = FOURCC('2', '4', 'B', 'G'),
	FOURCC_RAW = FOURCC('r', 'a', 'w', ' '),
	FOURCC_RGBA = FOURCC('R', 'G', 'B', 'A'),
	FOURCC_RGBP = FOURCC('R', 'G', 'B', 'P'),  // bgr565.
	FOURCC_RGBO = FOURCC('R', 'G', 'B', 'O'),  // abgr1555.
	FOURCC_R444 = FOURCC('R', '4', '4', '4'),  // argb4444.

											   // 4 Secondary RGB formats: 4 Bayer Patterns.
											   FOURCC_RGGB = FOURCC('R', 'G', 'G', 'B'),
											   FOURCC_BGGR = FOURCC('B', 'G', 'G', 'R'),
											   FOURCC_GRBG = FOURCC('G', 'R', 'B', 'G'),
											   FOURCC_GBRG = FOURCC('G', 'B', 'R', 'G'),

											   // 1 Primary Compressed YUV format.
											   FOURCC_MJPG = FOURCC('M', 'J', 'P', 'G'),

											   // 5 Auxiliary YUV variations: 3 with U and V planes are swapped, 1 Alias.
											   FOURCC_YV12 = FOURCC('Y', 'V', '1', '2'),
											   FOURCC_YV16 = FOURCC('Y', 'V', '1', '6'),
											   FOURCC_YV24 = FOURCC('Y', 'V', '2', '4'),
											   FOURCC_YU12 = FOURCC('Y', 'U', '1', '2'),  // Linux version of I420.
											   FOURCC_J420 = FOURCC('J', '4', '2', '0'),
											   FOURCC_J400 = FOURCC('J', '4', '0', '0'),

											   // 14 Auxiliary aliases.  CanonicalFourCC() maps these to canonical fourcc.
											   FOURCC_IYUV = FOURCC('I', 'Y', 'U', 'V'),  // Alias for I420.
											   FOURCC_YU16 = FOURCC('Y', 'U', '1', '6'),  // Alias for I422.
											   FOURCC_YU24 = FOURCC('Y', 'U', '2', '4'),  // Alias for I444.
											   FOURCC_YUYV = FOURCC('Y', 'U', 'Y', 'V'),  // Alias for YUY2.
											   FOURCC_YUVS = FOURCC('y', 'u', 'v', 's'),  // Alias for YUY2 on Mac.
											   FOURCC_HDYC = FOURCC('H', 'D', 'Y', 'C'),  // Alias for UYVY.
											   FOURCC_2VUY = FOURCC('2', 'v', 'u', 'y'),  // Alias for UYVY on Mac.
											   FOURCC_JPEG = FOURCC('J', 'P', 'E', 'G'),  // Alias for MJPG.
											   FOURCC_DMB1 = FOURCC('d', 'm', 'b', '1'),  // Alias for MJPG on Mac.
											   FOURCC_BA81 = FOURCC('B', 'A', '8', '1'),  // Alias for BGGR.
											   FOURCC_RGB3 = FOURCC('R', 'G', 'B', '3'),  // Alias for RAW.
											   FOURCC_BGR3 = FOURCC('B', 'G', 'R', '3'),  // Alias for 24BG.
											   FOURCC_CM32 = FOURCC(0, 0, 0, 32),  // Alias for BGRA kCMPixelFormat_32ARGB
											   FOURCC_CM24 = FOURCC(0, 0, 0, 24),  // Alias for RAW kCMPixelFormat_24RGB

																				   // 1 Auxiliary compressed YUV format set aside for capturer.
																				   FOURCC_H264 = FOURCC('H', '2', '6', '4'),
};

pulsar::PulsarDesktopCapturer *cap;
class capCallback : public webrtc::DesktopCapturer::Callback
{
public:
	virtual void OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame);
};

void capCallback::OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame)
{
	std::cout << "okok" << std::endl;
	const int32_t width = frame->size().width();
	const int32_t height = frame->size().height();
	int stride_y = width;
	int stride_uv = (width + 1) / 2;
	int target_width = width;
	int target_height = height;
	rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(
		target_width, target_height, stride_y, stride_uv, stride_uv);
	const int conversionResult = ConvertToI420(
		webrtc::kARGB, frame->data(), 0, 0,  // No cropping
		width, height, width * height * 4,
		webrtc::kVideoRotation_0, buffer.get());
	if (conversionResult < 0)
	{
		LOG(LS_ERROR) << "Failed to convert capture frame from type ";
		return;
	}

	webrtc::VideoFrame captureFrame(
		buffer, 0, rtc::TimeMillis(),
		webrtc::kVideoRotation_0);
	captureFrame.set_ntp_time_ms(0);
	
	cap->OnIncomingCapturedFrame(0, captureFrame);
}

void CaptureThread()
{
	std::cout << "hahaha" << std::endl;
	webrtc::DesktopCaptureOptions options =
		webrtc::DesktopCaptureOptions::CreateDefault();
	std::unique_ptr<webrtc::ScreenCapturer> screen_capturer(
		webrtc::ScreenCapturer::Create(options));

	std::unique_ptr<webrtc::DesktopCapturer> capturer;
	if (screen_capturer && screen_capturer->SelectScreen(0)) {
		capturer.reset(new webrtc::DesktopAndCursorComposer(
			screen_capturer.release(),
			webrtc::MouseCursorMonitor::CreateForScreen(options, 0)));
	}
	capCallback cb;
	capturer->Start(&cb);
	while (1)
	{
		capturer->CaptureFrame();
	}
}

namespace pulsar {
	PulsarDesktopCapturer::PulsarDesktopCapturer() {
		cap = this;
		std::vector<cricket::VideoFormat> fmt;
		cricket::VideoFormat format;
		format.width = 500;
		format.height = 500;
		format.fourcc = FOURCC_I420;
		format.interval = 50;
		fmt.push_back(format);
		SetSupportedFormats(fmt);
	}
	PulsarDesktopCapturer::~PulsarDesktopCapturer() {}
	struct kVideoFourCCEntry {
		uint32_t fourcc;
		webrtc::RawVideoType webrtc_type;
	};
	static kVideoFourCCEntry kSupportedFourCCs[] = {
		{ FOURCC_I420, webrtc::kVideoI420 },   // 12 bpp, no conversion.
		{ FOURCC_YV12, webrtc::kVideoYV12 },   // 12 bpp, no conversion.
		{ FOURCC_YUY2, webrtc::kVideoYUY2 },   // 16 bpp, fast conversion.
		{ FOURCC_UYVY, webrtc::kVideoUYVY },   // 16 bpp, fast conversion.
		{ FOURCC_NV12, webrtc::kVideoNV12 },   // 12 bpp, fast conversion.
		{ FOURCC_NV21, webrtc::kVideoNV21 },   // 12 bpp, fast conversion.
		{ FOURCC_MJPG, webrtc::kVideoMJPEG },  // compressed, slow conversion.
		{ FOURCC_ARGB, webrtc::kVideoARGB },   // 32 bpp, slow conversion.
		{ FOURCC_24BG, webrtc::kVideoRGB24 },  // 24 bpp, slow conversion.
	};

	static bool FormatToCapability(const cricket::VideoFormat& format,
		webrtc::VideoCaptureCapability* cap) {
		webrtc::RawVideoType webrtc_type = webrtc::kVideoUnknown;
		for (size_t i = 0; i < arraysize(kSupportedFourCCs); ++i) {
			if (kSupportedFourCCs[i].fourcc == format.fourcc) {
				webrtc_type = kSupportedFourCCs[i].webrtc_type;
				break;
			}
		}
		if (webrtc_type == webrtc::kVideoUnknown) {
			return false;
		}

		cap->width = format.width;
		cap->height = format.height;
		cap->maxFPS = cricket::VideoFormat::IntervalToFps(format.interval);
		cap->expectedCaptureDelay = 0;
		cap->rawType = webrtc_type;
		cap->codecType = webrtc::kVideoCodecUnknown;
		cap->interlaced = false;
		return true;
	}

	cricket::CaptureState PulsarDesktopCapturer::Start(const cricket::VideoFormat &capture_format)
	{


		start_thread_ = rtc::Thread::Current();
		//RTC_DCHECK(!async_invoker_);
		async_invoker_.reset(new rtc::AsyncInvoker());
		//captured_frames_ = 0;

		SetCaptureFormat(&capture_format);

		webrtc::VideoCaptureCapability cap;
		if (!FormatToCapability(capture_format, &cap)) {
			LOG(LS_ERROR) << "Invalid capture format specified";
			return cricket::CS_FAILED;
		}

		int64_t start = rtc::TimeMillis();
		//module_->RegisterCaptureDataCallback(*this);
		/*if (module_->StartCapture(cap) != 0) {
			LOG(LS_ERROR) << "Camera '" << GetId() << "' failed to start";
			module_->DeRegisterCaptureDataCallback();
			async_invoker_.reset();
			SetCaptureFormat(nullptr);
			start_thread_ = nullptr;
			return cricket::CS_FAILED;
		}*/

		LOG(LS_INFO) << "Camera '" << GetId() << "' started with format "
			<< capture_format.ToString() << ", elapsed time "
			<< rtc::TimeSince(start) << " ms";

		SetCaptureState(cricket::CS_RUNNING);
		std::thread captureThread(CaptureThread);
		return cricket::CS_RUNNING;
	}



	void PulsarDesktopCapturer::Stop()
	{
		SetCaptureState(cricket::CS_STOPPED);
	}

	bool PulsarDesktopCapturer::IsRunning()
	{
		return true;
	}

	bool PulsarDesktopCapturer::IsScreencast() const
	{
		return true;
	}

	bool PulsarDesktopCapturer::GetPreferredFourccs(std::vector<uint32_t>* fourccs)
	{
		return true;
	}

	void PulsarDesktopCapturer::OnIncomingCapturedFrame(
		const int32_t id,
		const webrtc::VideoFrame& sample) {
		// This can only happen between Start() and Stop().
		RTC_DCHECK(start_thread_);
		RTC_DCHECK(async_invoker_);


		OnFrame(cricket::WebRtcVideoFrame(
			sample.video_frame_buffer(), sample.rotation(),
			sample.render_time_ms() * rtc::kNumMicrosecsPerMillisec),
			sample.width(), sample.height());
	}

	void PulsarDesktopCapturer::OnCaptureDelayChanged(const int32_t id,
		const int32_t delay) {
		LOG(LS_INFO) << "Capture delay changed to " << delay << " ms";
	}
}