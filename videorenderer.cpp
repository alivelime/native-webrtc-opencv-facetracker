#include "videorenderer.h"
#include "webrtc/base/logging.h"
#include "libyuv/convert_argb.h"


VideoRenderer::VideoRenderer(int w, int h,
    rtc::scoped_refptr<webrtc::VideoTrackInterface> track_to_render,
    CustomVideoCapturer& cap)
  : rendered_track(track_to_render), width(w), height(h), capturer(cap) {

  rendered_track->AddOrUpdateSink(this, rtc::VideoSinkWants());
}

VideoRenderer::~VideoRenderer() {
  rendered_track->RemoveSink(this);
}

void VideoRenderer::SetSize(int w, int h) {
  LOG(INFO) << "VideoRenderer::SetSize(" << w << "," << h << ")";

  if (width == w && height == h) {
    return;
  }

  width = w;
  height = h;

  image.reset(new std::uint8_t[width * height * 4]);
}

void VideoRenderer::OnFrame(const webrtc::VideoFrame& video_frame) {

  LOG(INFO) << "VideoRenderer::OnFrame()";

  rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(video_frame.video_frame_buffer()->ToI420());

  SetSize(buffer->width(), buffer->height());

  libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(),
                     buffer->DataU(), buffer->StrideU(),
                     buffer->DataV(), buffer->StrideV(),
                     image.get(),
                     width * 4,
                     buffer->width(), buffer->height());

  capturer.Render(image.get(), width, height);
}

