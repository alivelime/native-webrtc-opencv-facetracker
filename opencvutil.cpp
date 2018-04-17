#include "opencvutil.h"
#include <unistd.h>

/**------------------------------------------------------------*
* @fn          OpenCVのピクチャーインピクチャ http://qiita.com/kinojp/items/d2d9a68a962b34b62888
* @brief       画像内に画像を貼り付ける（位置を座標で指定）
* @par[in ]    srcImg  背景画像
* @par[in ]    smallImg    前景画像
* @par[in ]    p0  前景画像の左上座標
* @par[in ]    p1  前景画像の右下座標
*------------------------------------------------------------*/
void drawPinP(cv::Mat& dstImg, const cv::Mat &srcImg, const cv::Mat &smallImg, const cv::Point2f p0, const cv::Point2f p1)
{
	//背景画像の作成
	srcImg.copyTo(dstImg);

	//３組の対応点を作成
	std::vector<cv::Point2f> src, dst;
	src.push_back(cv::Point2f(0, 0));
	src.push_back(cv::Point2f(smallImg.cols, 0));
	src.push_back(cv::Point2f(smallImg.cols, smallImg.rows));

	dst.push_back(p0);
	dst.push_back(cv::Point2f(p1.x, p0.y));
	dst.push_back(p1);

	//前景画像の変形行列
	cv::Mat mat = cv::getAffineTransform(src, dst);

	//アフィン変換の実行
	cv::warpAffine(smallImg, dstImg, mat, dstImg.size(), CV_INTER_LINEAR, cv::BORDER_TRANSPARENT);
}

/**-----------------------------------------------------------*
 * @fn  DrawTransPinP http://kurino.xii.jp/Gallery/Computer/Code/ocv/transparent.html
 * @brief   透過画像を重ねて描画する
 * @param[out] img_dst
 * @param[in ] transImg 前景画像。アルファチャンネル付きであること(CV_8UC4)
 * @param[in ] baseImg  背景画像。アルファチャンネル不要(CV_8UC4)
 *------------------------------------------------------------*/
void drawTransPinP(cv::Mat &img_dst, const cv::Mat transImg, const cv::Mat baseImg, std::vector<cv::Point2f> tgtPt)
{
    cv::Mat img_aaa, img_1ma;
    std::vector<cv::Mat> planes_rgba, planes_aaa, planes_1ma;
    int maxVal = pow(2, 8*baseImg.elemSize1())-1;
 
    //透過画像はRGBA, 背景画像はRGBのみ許容。ビット深度が同じ画像のみ許容
    if(transImg.data==NULL || baseImg.data==NULL ||
        transImg.channels() < 4 || baseImg.channels() < 4 ||
        (transImg.elemSize1() != baseImg.elemSize1())  ||
        tgtPt.size() < 4
    ) {
      std::cout << "trans image is wrong." << std::endl;
        return;
    }
 
    //変形行列を作成
    std::vector<cv::Point2f>srcPt;
    srcPt.push_back( cv::Point2f(0, 0) );
    srcPt.push_back( cv::Point2f(transImg.cols-1, 0) );
    srcPt.push_back( cv::Point2f(transImg.cols-1, transImg.rows-1) );
    srcPt.push_back( cv::Point2f(0, transImg.rows-1) );
    cv::Mat mat = cv::getPerspectiveTransform(srcPt, tgtPt);
 
    //出力画像と同じ幅・高さのアルファ付き画像を作成
    cv::Mat alpha0(baseImg.rows, baseImg.cols, transImg.type() );
    alpha0 = cv::Scalar::all(0);
    cv::warpPerspective(transImg, alpha0, mat,alpha0.size(), cv::INTER_CUBIC, cv::BORDER_TRANSPARENT);
 
    //チャンネルに分解
    cv::split(alpha0, planes_rgba);
 
    //RGBA画像からアルファチャンネル抽出   
    planes_aaa.push_back(planes_rgba[3]);
    planes_aaa.push_back(planes_rgba[3]);
    planes_aaa.push_back(planes_rgba[3]);
    planes_aaa.push_back(planes_rgba[3]);
    merge(planes_aaa, img_aaa);
 
    //背景用アルファチャンネル   
    planes_1ma.push_back(maxVal-planes_rgba[3]);
    planes_1ma.push_back(maxVal-planes_rgba[3]);
    planes_1ma.push_back(maxVal-planes_rgba[3]);
    planes_1ma.push_back(maxVal-planes_rgba[3]);
    merge(planes_1ma, img_1ma);
 
    img_dst = alpha0.mul(img_aaa, 1.0/(double)maxVal) + baseImg.mul(img_1ma, 1.0/(double)maxVal);
}

