#include "opencvdetector.h"
#include "opencvutil.h"

OpenCVDetector::OpenCVDetector(const std::string& image_fn, const std::string& cascade_fn,
    OffsetScale os, float scale, int neighbor
  ) : offsetScale(os), scaleFactor(scale), minNeighbors(neighbor) {
	if (!detector.load("data/" + cascade_fn)) {
    std::cout << "fail to load: data/" << cascade_fn << std::endl;
    std::exit(1);
	}
  image = cv::imread("image/" + image_fn, -1);
  if (image.empty()) {
    std::cout << "fail to load: image/" + image_fn << std::endl;
    std::exit(1);
  }
  if (image.channels() == 3) {
		cv::cvtColor(image, image, CV_BGR2BGRA );
	}
}
OpenCVDetector::~OpenCVDetector() {
}

int OpenCVDetector::Draw(cv::Mat target) {

  int width = target.cols;
  int height = target.rows;
  
  auto draw = [&](std::vector<cv::Rect> results) {
    for (const auto& r : results) {
      int center_x = (r.x + r.width / 2) + (r.width * offsetScale.x);
      int center_y = (r.y + r.height / 2) + (r.height * offsetScale.y);
      int left = center_x - (r.width * offsetScale.width / 2);
      int right = center_x + (r.width + offsetScale.width / 2);
      int top = center_y - (r.height * offsetScale.height / 2);
      int bottom = center_y + (r.height + offsetScale.height / 2);
      
      // draw area border
      if (image.channels() == 3) {
       drawPinP(target, target, image, cv::Point2f(left, top), cv::Point2f(right, bottom));
      } else {
        // draw pictue png
        std::vector<cv::Point2f> target_point;
        target_point.push_back(cv::Point2f(left , top   )); // top left
        target_point.push_back(cv::Point2f(right, top   )); // top right
        target_point.push_back(cv::Point2f(right, bottom)); // bottom right;
        target_point.push_back(cv::Point2f(left , bottom)); // bottom left
     
        drawTransPinP(target, image, target, target_point);
      }
    }
  };

	std::vector<cv::Rect> results;
	detector.detectMultiScale(target, results, scaleFactor, minNeighbors, 0,
      cv::Size(width * 0.1, height * 0.1));
  if (results.size() > 0) {
    draw(results);
    last_found_results = results;
    last_found_time = std::chrono::system_clock::now();
    return last_found_results.size();
  } else if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()
                                                              - last_found_time).count() <= 1000) {
    // if can not found face. but while 1 second, draw picture.
    draw(last_found_results);
    return last_found_results.size();
  }

  return 0;
}

