
#include "single_calibrate.h"
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

// ��ȡ��ɫ�ͻҶ�ͼ��
void SingleCalibrate::GetImages(void)
{
	/* �궨����ͼ���ļ���·�� */
	ifstream fin(inputImagePath_);
	string filename;

	while (getline(fin, filename))
	{
		cout << filename << endl;
		cv::Mat rgbImage = cv::imread(filename);
		if (!rgbImage.empty()) {
			// ��ȡ��ɫRGBͼ��
			rgbImages_.push_back(rgbImage);

			// ת��Ϊ�Ҷ�ͼ��
			cv::Mat grayImage;
			cv::cvtColor(rgbImage, grayImage, CV_RGB2GRAY);
			grayImages_.push_back(grayImage);
		}
	}

	// ͼ����Ŀ
	imageCount_ = rgbImages_.size();

	// ����ͼ�������ͬ�Ŀ�Ⱥ͸߶�
	if (imageCount_ > 0) {
		imageWidth_ = rgbImages_[0].cols;
		imageHeight_ = rgbImages_[0].rows;
	}
}

// ��ȡͼ���еĶ�ά�ǵ㣬��������imageCorners2D_��
void SingleCalibrate::ExtractImageCorners2D(void)
{
	std::vector<cv::Point2f> tempImageCorners2D;
	for (int i = 0; i < imageCount_; ++i) {
		// ������ȡ���ؼ����ȵĽǵ�
		if (0 == findChessboardCorners(rgbImages_[i], cv::Size(cornerNumInWidth_, cornerNumInHeight_), tempImageCorners2D))
		{
			cout << "cannot get image corners 2D: " << i << endl;
			continue;
		}

		// ��ȡ���ȸ��ߵĽǵ㣨�����ؼ���
		cv::cornerSubPix(grayImages_[i], tempImageCorners2D, cv::Size(11, 11), cv::Size(-1, -1),
			cv::TermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 30, 0.01));

		imageCorners2D_.push_back(tempImageCorners2D);
		// ���Ʋ���ʾ�ǵ�
		drawChessboardCorners(rgbImages_[i], cv::Size(cornerNumInWidth_, cornerNumInHeight_), tempImageCorners2D, true);
		imshow("Camera Calibration", rgbImages_[i]);
		waitKey(10);
	}
}

// ����ǵ���ʵ��3ά�������꣨Z��Ϊ0������������realCorners3D_��
void SingleCalibrate::CalcRealCorners3D(void)
{
	for (int n = 0; n < imageCount_; ++n)
	{
		vector<Point3f> tempRealCorners3D;
		for (int h = 0; h < cornerNumInHeight_; ++h)
		{
			for (int w = 0; w < cornerNumInWidth_; ++w)
			{
				Point3f realPoint;
				/* ����궨�������������ϵ��z=0��ƽ���� */
				realPoint.y = h * lengthOfSquare_;// lengthOfSquare_Ϊ��ӡ������ֽ�ϣ��ڰ������θ��ӵ�ʵ�ʿ��
				realPoint.x = w * lengthOfSquare_;
				realPoint.z = 0;
				tempRealCorners3D.push_back(realPoint);
			}
		}
		realCorners3D_.push_back(tempRealCorners3D);
	}
}

// ���ݶ�ά����ά�ǵ���Ϣ���������Ŀ�������ת����cameraMatrix_��ƽ�ƾ���distCoeffs_
void SingleCalibrate::SingleCalibrateCamera(void)
{
	cameraMatrix_ = cv::Mat(3, 3, CV_32FC1, Scalar::all(0));
	distCoeffs_ = cv::Mat(1, 5, CV_32FC1, Scalar::all(0));
	calibrateCamera(realCorners3D_, imageCorners2D_, cv::Size(imageWidth_, imageHeight_), cameraMatrix_, distCoeffs_, rvecsMat_, tvecsMat_, 0);
}

// ������Ŀ����궨�����ݲ���Ҫ��ϸ�о���
void SingleCalibrate::Estimate(void)
{
	std::string outputCalibrateParam = "D:/double_camera/left/caliberation_result.xml";
	FileStorage fs(outputCalibrateParam, FileStorage::WRITE);
	double total_err = 0.0; /* ����ͼ���ƽ�������ܺ� */
	double err = 0.0; /* ÿ��ͼ���ƽ����� */
	vector<Point2f> image_points2; /* �������¼���õ���ͶӰ�� */

	for (int i = 0; i < imageCount_; i++)
	{
		vector<Point3f> tempPointSet = realCorners3D_[i];
		/* ͨ���õ������������������Կռ����ά���������ͶӰ���㣬�õ��µ�ͶӰ�� */
		projectPoints(tempPointSet, rvecsMat_[i], tvecsMat_[i], cameraMatrix_, distCoeffs_, image_points2);
		/* �����µ�ͶӰ��;ɵ�ͶӰ��֮������*/
		vector<Point2f> tempImagePoint = imageCorners2D_[i];
		Mat tempImagePointMat = Mat(1, tempImagePoint.size(), CV_32FC2);
		Mat image_points2Mat = Mat(1, image_points2.size(), CV_32FC2);
		for (int j = 0; j < tempImagePoint.size(); j++)
		{
			image_points2Mat.at<Vec2f>(0, j) = Vec2f(image_points2[j].x, image_points2[j].y);
			tempImagePointMat.at<Vec2f>(0, j) = Vec2f(tempImagePoint[j].x, tempImagePoint[j].y);
		}
		err = norm(image_points2Mat, tempImagePointMat, NORM_L2);
		total_err += err /= (cornerNumInWidth_ * cornerNumInHeight_);
		std::cout << "��" << i + 1 << "��ͼ���ƽ����" << err << "����" << endl;
	}

	std::cout << "��ʾ���������" << std::endl;
	for (int k = 0; k < imageCount_; ++k)
	{
		cv::namedWindow("src");
		cv::namedWindow("dst");
		cv::moveWindow("src", 100, 0);
		cv::moveWindow("dst", 800, 0);

		Mat imageInput = rgbImages_[k];
		cv::imshow("src", imageInput);

		cv::Mat imageOutput;
		undistort(imageInput, imageOutput, cameraMatrix_, distCoeffs_);

		cv::imshow("dst", imageOutput);
		cv::waitKey(500);
	}
	std::cout << "����ƽ����" << total_err / imageCount_ << "����" << endl;
	std::cout << "������ɣ�" << endl;
	//���涨����  	
	std::cout << "��ʼ���涨����������������" << endl;
	Mat rotation_matrix = Mat(3, 3, CV_32FC1, Scalar::all(0)); /* ����ÿ��ͼ�����ת���� */
	fs << "cameraMatrix" << cameraMatrix_;
	fs << "distCoeffs" << distCoeffs_;
	std::cout << "��ɱ���" << endl;
}


// ��Ŀ����궨��ȫ����
void SingleCalibrate::ProcessCalibrate(void)
{
	GetImages();
	ExtractImageCorners2D();
	CalcRealCorners3D();
	SingleCalibrateCamera();
}


//// ���Ժ��������ԣ�
//void new_one()
//{
//	std::string inputImagePath = "D:/double_camera/calib_imgs/calib_data_left.txt";
//	int lengthOfSquare = 10;
//	int cornerNumInWidth = 6;
//	int cornerNumInHeight = 9;
//
//	SingleCalibrate singleCalibrate(inputImagePath, cornerNumInWidth, cornerNumInHeight, lengthOfSquare);
//	singleCalibrate.ProcessCalibrate();
//	singleCalibrate.Estimate();
//}

//void main()
//{
//	//original();
//	new_one();
//}