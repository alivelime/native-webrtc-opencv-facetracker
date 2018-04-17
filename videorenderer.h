#ifndef OMEKASHI_VIDEORENDERER_H
#define  OMEKASHI_VIDEORENDERER_H

#include "customvideocapturer.h"

#include <cstdint>
#include "webrtc/api/video/video_frame.h"
#include "webrtc/api/mediastreaminterface.h"

class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
  public:
    explicit VideoRenderer(int width, int height,
        rtc::scoped_refptr<webrtc::VideoTrackInterface> track_to_render,
        CustomVideoCapturer& capturer);
    virtual ~VideoRenderer();

    void OnFrame(const webrtc::VideoFrame& frame) override;

  protected:
    void SetSize(int width, int height);
    std::unique_ptr<uint8_t[]> image;
    rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track;

    int width;
    int height;

    CustomVideoCapturer& capturer;
};
#endif // OMEKASHI_VIDEORENDERER_H
