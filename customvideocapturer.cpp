#include "customvideocapturer.h"

#include <memory>

#include "webrtc/api/video/i420_buffer.h"
#include "webrtc/common_types.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/base/logging.h"
 
 
using std::endl;
 
CustomVideoCapturer::CustomVideoCapturer(const Session& s)
  : now_rendering(false)
  , renderer(RendererFactory::Create())
  , session(s)
  , start()
  , end(std::chrono::system_clock::now())
  , frame_timer(std::chrono::system_clock::now())
  , frame_counter(0)
{
}
 
CustomVideoCapturer::~CustomVideoCapturer()
{
}
 
void CustomVideoCapturer::Render(std::uint8_t* image, const int width, const int height) {

  if (now_rendering) {
    
    // if rendering is too slow then skip.
    start = std::chrono::system_clock::now();
    if (std::chrono::duration_cast<std::chrono::microseconds>(start - end).count() < 10000) {
      LOG(INFO) << "rendering is too slow, so skiped.";
      return;
    }

    // frame rate count;
    frame_counter++;
    if (std::chrono::duration_cast<std::chrono::milliseconds>(start - frame_timer).count() >= 1000) {
      renderer->SetFrameRate(frame_counter);

      frame_timer = std::chrono::system_clock::now();
      frame_counter = 0;
    }

    renderer->PutImage(image, width, height);
    renderer->Filter(session.mode);

    int buf_width = renderer->GetWidth();
    int buf_height = renderer->GetHeight();

//    rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(width, height);
    rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(
        buf_width, buf_height, buf_width, (buf_width + 1) / 2, (buf_width + 1) / 2);
    const int conversionResult = webrtc::ConvertToI420(
        webrtc::VideoType::kARGB, renderer->Image(), 0, 0,  // No cropping
        buf_width, buf_height, CalcBufferSize(webrtc::VideoType::kARGB, buf_width, buf_height),
        webrtc::kVideoRotation_0, buffer.get());

    if (conversionResult < 0) {
      LOG(LS_ERROR) << "Failed to convert capture frame from type "
                    << static_cast<int>(webrtc::VideoType::kARGB) << "to I420.";
      return ;
    }

    webrtc::VideoFrame frame(buffer, 0, rtc::TimeMillis(), webrtc::kVideoRotation_0);
    frame.set_ntp_time_ms(0);

    OnFrame(frame, buf_width, buf_height);

    end = std::chrono::system_clock::now();
    LOG(INFO) << "frame used "
      << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
      << " micro sec.";
  } else {
    LOG(INFO) << "render() start but now_rendering is false.";
  }
}

cricket::CaptureState CustomVideoCapturer::Start(const cricket::VideoFormat& capture_format)
{
    LOG(INFO) << "CustomVideoCapture start.";
    if (capture_state() == cricket::CS_RUNNING) {
        LOG(LS_ERROR) << "Start called when it's already started.";
        return capture_state();
    }
 
    now_rendering = true;
 
    SetCaptureFormat(&capture_format);
    return cricket::CS_RUNNING;
}
 
void CustomVideoCapturer::Stop()
{
    LOG(INFO) << "CustomVideoCapture::Stop()";
    now_rendering = false;
    if (capture_state() == cricket::CS_STOPPED) {
        LOG(LS_ERROR) << "Stop called when it's already stopped.";
        return;
    }
 
    SetCaptureFormat(NULL);
    SetCaptureState(cricket::CS_STOPPED);
}
 
 
bool CustomVideoCapturer::IsRunning()
{
    return capture_state() == cricket::CS_RUNNING;
}
 
bool CustomVideoCapturer::GetPreferredFourccs(std::vector<uint32_t>* fourccs)
{
    if (!fourccs)
        return false;
    fourccs->push_back(cricket::FOURCC_I420);
    return true;
}
 
bool CustomVideoCapturer::GetBestCaptureFormat(const cricket::VideoFormat& desired, cricket::VideoFormat* best_format)
{
    if (!best_format)
        return false;
 
    // Use the desired format as the best format.
    best_format->width = desired.width;
    best_format->height = desired.height;
    best_format->fourcc = cricket::FOURCC_I420;
    best_format->interval = desired.interval;
    return true;
}
 
bool CustomVideoCapturer::IsScreencast() const
{
    return false;
}
 
