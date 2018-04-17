#include "opencvrenderer.h"
#include "opencvutil.h"

#include <iostream>
 

OpenCVRenderer::OpenCVRenderer() 
  : face("kaicho.jpg")
  /*
  : upperbody("boy.png", "haarcascade_upperbody.xml", {1,1,0,0}, 1.1, 20)
  , face("kaicho.jpg", "haarcascade_frontalface_alt2.xml", {1,1,0,0}, 1.1, 3)
  , smile("smile.png", "haarcascade_smile.xml", {1,1,0,0}, 1.1, 20)
  , eye("eye.png", "haarcascade_eye_tree_eyeglasses.xml", {1,1,0,0}, 1.1, 20) 
  , mustache("mustache.png", "haarcascade_mcs_mouth.xml", {3.0,6.0,0,0}, 1.1, 20) 
  */
{

  waitPicture = cv::imread("image/wait_picture.png", -1);
  if (waitPicture.empty()) {
    waitPicture = cv::imread("image/wait_picture.jpg", -1);
    if (waitPicture.empty()) {
      std::cout << "fail to load: image/wait_picture.jpg or .png" << std::endl;
      std::exit(1);
    }
  }
  if (waitPicture.channels() == 3) {
		cv::cvtColor(waitPicture, waitPicture, CV_BGR2BGRA );
  }

}
OpenCVRenderer::~OpenCVRenderer() {
}

void OpenCVRenderer::PutImage(std::uint8_t* image, int w, int h) {

  width = w, height = h;
  buffer = cv::Mat(cv::Size(width, height), CV_8UC4, image);

  // TODO rebuild opencv3.2 on webrtc compiler set for link error. undefined reference to imwrite(...);
  /*
  std::vector<int> compression_params;
  compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
  compression_params.push_back(9);
  imwrite("alpha.png", buffer, compression_params);
  */

}

std::uint8_t* OpenCVRenderer::Image() {

   cv::putText(buffer, std::to_string(frame_rate), cv::Point(10,50), cv::FONT_HERSHEY_SIMPLEX, 2,
		 					 cv::Scalar(101,45,75, 255), 3, CV_AA);
   cv::putText(buffer, std::to_string(frame_rate), cv::Point(10,50), cv::FONT_HERSHEY_SIMPLEX, 1.2,
		 					 cv::Scalar(194,179,244, 255), 2, CV_AA);

  return buffer.ptr();
}

void OpenCVRenderer::Filter(const RenderMode& mode) {

  /*
  if (!mode.onAir) {
    cv::resize(waitPicture, buffer, buffer.size());
  } else if (mode.isKaicho) {
    if (!face.Draw(buffer)) {
      cv::resize(waitPicture, buffer, buffer.size());
    }
  } else {
    int found = 0;
    if(mode.useEye) found += eye.Draw(buffer);
    if(mode.useMustache) found += mustache.Draw(buffer);

    if (found == 0) {
      cv::blur(buffer, buffer, cv::Size(128, 128));
    }
  }
  */

  if (!mode.onAir) {
    cv::resize(waitPicture, buffer, buffer.size());
  } else {
    face.Draw(buffer);
  }

  /*
	buffer->forEach<cv::Vec4b>([](cv::Vec4b &x, const int *position) {
    const std::uint8_t grey = x[0] * 0.1 + x[1] * 0.6 + x[3] * 0.1;
    x[0] = (grey < 32 ? 0 : grey - 32);
    x[1] = grey;
    x[2] = (grey > 225 ? 255: grey + 32);
	});
  */
}


