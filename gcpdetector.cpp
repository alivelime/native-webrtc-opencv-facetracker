#include "gcpdetector.h"
#include "opencvutil.h"
#include "google/cloud/vision/v1/image_annotator.grpc.pb.h"

#include <sstream>
#include <grpc++/grpc++.h>
#include <chrono>

using namespace google::cloud::vision::v1;

GCPDetector::GCPDetector(const std::string& image_fn)
{
  image = cv::imread("image/" + image_fn, -1);
  if (image.empty()) {
    std::cout << "fail to load: image/" + image_fn << std::endl;
    std::exit(1);
  }
  if (image.channels() == 3) {
		cv::cvtColor(image, image, CV_BGR2BGRA );
	}
}
GCPDetector::~GCPDetector() {
}

int GCPDetector::Draw(cv::Mat target) {

  int width = target.cols;
  int height = target.rows;

  auto creds = grpc::GoogleDefaultCredentials();
  auto channel = grpc::CreateChannel("vision.googleapis.com", creds);
  auto detector(ImageAnnotator::NewStub(channel));

  BatchAnnotateImagesRequest batch;
  auto request = batch.add_requests();
  auto feature = request->add_features();
  feature->set_max_results(0);
  feature->set_type(Feature_Type::Feature_Type_FACE_DETECTION);
  
  std::vector<uchar> buf;//buffer for coding
  {
    std::vector<int> param(2);
    param[0]=CV_IMWRITE_PNG_COMPRESSION;
    param[1]=3;//default(3)  0-9.
    auto start = std::chrono::system_clock::now();
    cv::imencode(".png", target, buf, param);
    auto end = std::chrono::system_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
      << " micro sec."
      << " size: " << buf.size()
      << std::endl;

    std::ofstream fout;
    fout.open("test.png", std::ios::out|std::ios::binary|std::ios::trunc);
    fout.write((char*)&buf, buf.size());
		fout.close();
  }

  request->mutable_image()->set_content(std::string(buf.begin(), buf.end()));

  grpc::ClientContext context;
  grpc::CompletionQueue cq;

  BatchAnnotateImagesResponse response;
  grpc::Status status;
  status = detector->BatchAnnotateImages(&context, batch, &response);
  std::cout << status.error_code() << std::endl;
  std::cout << status.error_message() << std::endl;
  std::cout << status.error_details() << std::endl;
  std::cout << response.DebugString();

    for (const auto& face : response.responses()[0].face_annotations()) {
      // draw area border
      cv::Point pts[4];
      const cv::Point *ppt[1] = {pts}; 
      int i = 0;
      const int nums[] = {4};
      for (const auto& p : face.fd_bounding_poly().vertices()) {
        pts[i++] = cv::Point(p.x(), p.y());
      }
      cv::polylines(target, ppt, nums, 1, true, cv::Scalar(0,0,128,255));

      for (const auto& p : face.bounding_poly().vertices()) {
        pts[i++] = cv::Point(p.x(), p.y());
      }
      cv::polylines(target, ppt, nums, 1, true, cv::Scalar(0,0,192,255));

      std::cout << face.landmarks()[0].DebugString();
      for (const auto& parts : face.landmarks()) {
        cv::circle(target, cv::Point(parts.position().x(), parts.position().y()), 4,
            cv::Scalar(255,255,0,255), -1);
      }
    }

  return response.responses()[0].face_annotations_size();
}


