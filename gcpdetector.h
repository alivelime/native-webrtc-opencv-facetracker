#ifndef OMEKASHI_GCPDETECTOR_H
#define OMEKASHI_GCPDETECTOR_H

#include <chrono>

#include "opencv2/opencv.hpp"

class GCPDetector {
  cv::Mat image;

  std::chrono::system_clock::time_point last_found_time;

public:
  explicit GCPDetector (const std::string& image);
  virtual ~GCPDetector ();

  int Draw(cv::Mat target);
};



#endif // OMEKASHI_GCPDETECTOR_H
