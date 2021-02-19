#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <opencv2/imgproc/types_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "kcftracker.hpp"
#include "dirent.h"

#include "timer.hpp"
#include "../../darknet/include/yolo_v2_class.hpp"

#include "double_calibrate.h"

#define DETAIL

using namespace std;
using namespace cv;

typedef struct {
	int frameIndex;
	vector<int> coordinates;
} coordinatesInfo;



/// <summary>
/// ���ݷָ�����ַ����ָ�Ϊ���С��
/// </summary>
/// <param name="inputStr">����������ַ��������ָ�</param>
/// <param name="splitor">�ָ��</param>
/// <returns></returns>
static std::vector<std::string> split_string(const std::string& inputStr, const std::string& splitor)
{
	std::vector<std::string> result;
	std::string::size_type pos1, pos2;
	pos2 = inputStr.find(splitor);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		result.push_back(inputStr.substr(pos1, pos2 - pos1));

		pos1 = pos2 + splitor.size();
		pos2 = inputStr.find(splitor, pos1);
	}
	if (pos1 != inputStr.length())
		result.push_back(inputStr.substr(pos1));
	return result;
}



/// <summary>
/// �����������ο򣬼������ǵ�IOU
/// </summary>
/// <param name="left">����ĵ�һ�����ο�</param>
/// <param name="right">����ĵڶ������ο�</param>
/// <returns></returns>
float calcIOU(vector<int> left, vector<int> right)
{
	int maxX = std::max(left[0], right[0]);
	int maxY = std::max(left[1], right[1]);
	int minX = std::min(left[0] + left[2], right[0] + right[2]);
	int minY = std::min(left[1] + left[3], right[1] + right[3]);

	//maxX1 and maxY1 reuse 
	maxX = ((minX - maxX) > 0) ? (minX - maxX) : 0;
	maxY = ((minY - maxY) > 0) ? (minY - maxY) : 0;

	//IOU reuse for the area of two bboxintersection_width
	int intersection_area = maxX * maxY;

	int area_left = left[2] * left[3];
	int area_right = right[2] * right[3];

	float IOU = float(intersection_area) / (area_left + area_right - intersection_area);

	////��ʾ��һ�����ο����ϸ����
	//std::cout << "left:" << std::endl;
	//for (int i = 0; i < left.size(); ++i) {
	//	cout << left[i] << ", ";
	//}
	//std::cout << std::endl;

	////��ʾ�ڶ������ο����ϸ����
	//std::cout << "right:" << std::endl;
	//for (int i = 0; i < right.size(); ++i) {
	//	cout << right[i] << ", ";
	//}
	//std::cout << std::endl;

	////��ʾ��ϸ�����Ϣ
	//std::cout << "area_left:" << area_left << ", area_right:" << area_right << std::endl;
	//std::cout << "maxX:" << maxX << ", maxY:" << maxY << ", intersection_area:" << intersection_area << std::endl;
	//std::cout << "IOU: " << IOU << std::endl;

	return IOU;
}



/// <summary>
/// ��ȡyoloV4��KCF��ǵľ��ο����꼰֡��
/// </summary>
/// <param name="file_path">����Ϊ��ͬ���ʱ�����ɵ��ļ���interval_xx.txt</param>
/// <returns></returns>
vector<coordinatesInfo> extract_coordinates_from_file(std::string file_path)
{
	string temp;
	std::ifstream file(file_path);
	vector<coordinatesInfo> positions;
	while (getline(file, temp)) {
		vector<string> split1 = split_string(temp, ", ");
		if (split1.size() != 5) {
			break;
		}

		vector<string> split2 = split_string(split1[0], ":");

		coordinatesInfo temp_position;
		temp_position.frameIndex = std::atoi(split2[1].c_str());
		vector<int> coordinate;
		coordinate.push_back(std::atoi(split1[1].c_str()));
		coordinate.push_back(std::atoi(split1[2].c_str()));
		coordinate.push_back(std::atoi(split1[3].c_str()));
		coordinate.push_back(std::atoi(split1[4].c_str()));
		temp_position.coordinates = coordinate;

		positions.push_back(temp_position);
	}

	return positions;
}



/// <summary>
/// ��ȡyoloV4��KCF��һ����Ƶ�����ʱ��
/// </summary>
/// <param name="file_path">����Ϊ��ͬ���ʱ�����ɵ��ļ���interval_xx.txt</param>
/// <returns></returns>
float extract_time_from_file(std::string file_path)
{
	string temp;
	std::ifstream file(file_path);
	while (getline(file, temp)) {
		if (temp.find(",") != std::string::npos) {
			continue;
		}
		
		temp = temp.substr(temp.find(":") + 1);
		return std::atof(temp.c_str());
	}

	return 0;
}



/// <summary>
/// interval_1.txt��Ϊ��׼��interval_xx.txt��Ϊ�Ż������㲻ͬ���ʱ��ƽ��IOU׼ȷ�ʺ����ٱ���
/// </summary>
void generate_avgIOU_timeX_left()
{
	std::ofstream out("E:/second_data/interval/time_IOU_left.txt");
	std::string benchmark_file = "E:/second_data/interval/1_rect_left.txt";
	vector<coordinatesInfo> benchmark_coordinates = extract_coordinates_from_file(benchmark_file);
	float benchmark_time = extract_time_from_file(benchmark_file);

	for (int i = 2; i <= 100; ++i) {
		std::string compare_file = "E:/second_data/interval/" + to_string(i) + "_rect_left.txt";
		vector<coordinatesInfo> compare_coordinates = extract_coordinates_from_file(compare_file);

		float sum = 0;
		int totalNum = min(benchmark_coordinates.size(), compare_coordinates.size());
		for (int j = 0; j < totalNum; ++j) {
			float IOU = calcIOU(benchmark_coordinates[j].coordinates, compare_coordinates[j].coordinates);
			sum += IOU;
		}

		float avgIOU = sum / totalNum;
		cout << "avgIOU:" << avgIOU << std::endl;

		float compare_time = extract_time_from_file(compare_file);
		cout << benchmark_time << ", " << compare_time << ", " << benchmark_time / compare_time << std::endl;

		out << i << "," << avgIOU << "," << benchmark_time / compare_time << endl;
	}
}



/// <summary>
/// interval_1.txt��Ϊ��׼��interval_xx.txt��Ϊ�Ż������㲻ͬ���ʱ��ƽ��IOU׼ȷ�ʺ����ٱ���
/// </summary>
void generate_avgIOU_timeX_right()
{
	std::ofstream out("E:/second_data/interval/time_IOU_right.txt");
	std::string benchmark_file = "E:/second_data/interval/1_rect_right.txt";
	vector<coordinatesInfo> benchmark_coordinates = extract_coordinates_from_file(benchmark_file);
	float benchmark_time = extract_time_from_file(benchmark_file);

	for (int i = 2; i <= 100; ++i) {
		std::string compare_file = "E:/second_data/interval/" + to_string(i) + "_rect_right.txt";
		vector<coordinatesInfo> compare_coordinates = extract_coordinates_from_file(compare_file);

		float sum = 0;
		int totalNum = min(benchmark_coordinates.size(), compare_coordinates.size());
		for (int j = 0; j < totalNum; ++j) {
			float IOU = calcIOU(benchmark_coordinates[j].coordinates, compare_coordinates[j].coordinates);
			sum += IOU;
		}

		float avgIOU = sum / totalNum;
		cout << "avgIOU:" << avgIOU << std::endl;

		float compare_time = extract_time_from_file(compare_file);
		cout << benchmark_time << ", " << compare_time << ", " << benchmark_time / compare_time << std::endl;

		out << i << "," << avgIOU << "," << benchmark_time / compare_time << endl;
	}
}



/// <summary>
/// ��yoloV4������У��ҳ����Ϊ�������Ҿ��ο��������һ��
/// ���������û������ʱ�����ص�һ�����ο�
/// </summary>
/// <param name="pos">yoloV4����������п��ܰ���������ο�</param>
/// <returns></returns>
bbox_t get_max_one(std::vector<bbox_t> pos)
{
	bool find = false;
	bbox_t maxone;
	int max_area, area;
	for (int i = 0; i < pos.size(); i++) {
		// ��������
		if (pos[i].obj_id != 2) {
			continue;
		}

		area = pos[i].w * pos[i].h;

		if (find == false) {
			maxone = pos[i];
			max_area = area;
			find = true;
		}
		else {			
			if (area > max_area) {
				maxone = pos[i];
				max_area = area;				
			}
		}
	}

	//��yoloV4������У��ҳ����Ϊ�������Ҿ��ο��������һ��
	//���������û������ʱ�����ص�һ�����ο�
	return find == false ? pos[0] : maxone;
}



/// <summary>
/// ��������ͬʱ����ʱ����yoloV4������У��ҳ���ɫ�����ľ��ο�
/// </summary>
/// <param name="img">���������ͼƬ</param>
/// <param name="pos">yoloV4������ͼƬ���õ��Ľ�������п��ܰ���������ο򣬰�ɫ��������ɫ��������ɫ��������ɫ����</param>
/// <returns></returns>
bbox_t get_red_one(Mat img, std::vector<bbox_t> pos)
{
	Mat bgr, hsv;
	//��ɫͼ��ĻҶ�ֵ��һ��  
	img.convertTo(bgr, CV_32FC3, 1.0 / 255, 0);

	//��ɫ�ռ�ת��������ͨ��h����ʶ���ɫ
	cvtColor(bgr, hsv, COLOR_BGR2HSV);

	float max_ratio = 0;
	float max_index = 0;
	bool find = false;
	for (int i = 0; i < pos.size(); i++) {
		// ��������
		if (pos[i].obj_id != 2) {
			continue;
		}

		if (find == false) {
			int count = 0;
			for (int h = pos[i].y; h < pos[i].y + pos[i].h; h++)
			{
				for (int w = pos[i].x; w < pos[i].x + pos[i].w; w++)
				{
					// [330, 360]��ʾ��ɫ��Ӧ��hueֵ��Χ
					if (hsv.at<Vec3f>(h, w)[0] > 330 && hsv.at<Vec3f>(h, w)[0] < 360)
					{
						count++;
					}
				}
			}

			max_ratio = float(count) / pos[i].h / pos[i].w;
			max_index = i;
		}
		else {
			int count = 0;
			for (int h = pos[i].y; h < pos[i].y + pos[i].h; h++)
			{
				for (int w = pos[i].x; w < pos[i].x + pos[i].w; w++)
				{
					// [330, 360]��ʾ��ɫ��Ӧ��hueֵ��Χ
					if (hsv.at<Vec3f>(h, w)[0] > 330 && hsv.at<Vec3f>(h, w)[0] < 360)
					{
						count++;
					}
				}
			}

			// ��ɫռ�����ģ����Ǻ�ɫ����
			float ratio = float(count) / pos[i].h / pos[i].w;
			if (ratio > max_ratio) {
				max_ratio = ratio;
				max_index = i;
			}
		}
	}

	return pos[max_index];
}



/// <summary>
/// ���������Ƶ�У�ÿ���5֡����һ��ͼƬ��Ȼ���ֶ�����ɸѡ�����������궨���ͼƬ
/// </summary>
/// <returns></returns>
int generateCalibrateImages()
{
	// ����Ƶ�ļ�
	VideoCapture capture("E:/second_data/ttt4.avi");

	// �ж��Ƿ�򿪳ɹ�
	if (!capture.isOpened())
	{
		cout << "open camera failed. " << endl;
		return -1;
	}

	int count = 0;
	while (true)
	{
		// ��ȡͼ��֡��frame
		Mat frame;
		capture >> frame;

		// ͼ��֡��Ϊ��ʱ��ÿ���5֡������һ��ͼƬ
		if (!frame.empty())
		{
			imshow("camera", frame);

			if (count % 5 == 0) {
				cv::imwrite("E:/second_data/calibrate/calibrate_" + to_string(count) + ".jpg", frame);
			}

			count++;
			waitKey(10);
		}
	}

	return 0;
}



/// <summary>
/// ��2560*720�ı궨��ͼƬ�����Ϊ����1280*720��ͼƬ
/// </summary>
void splitCalibrateImages()
{
	string inputImagePath = "E:/second_data/calibrate/1/select.txt";
	ifstream fin(inputImagePath);

	string leftImagePath = "E:/second_data/calibrate/1/left/left.txt";
	string rightImagePath = "E:/second_data/calibrate/1/right/right.txt";
	ofstream outLeft(leftImagePath);
	ofstream outRight(rightImagePath);

	string filename;
	while (getline(fin, filename))
	{
		cout << filename << endl;
		cv::Mat image = cv::imread(filename);
		if (!image.empty()) {
			string name = filename.substr(27);
			name = name.substr(0, name.length() - 4);
			cout << "name:" << name << endl;
			Rect rectLeft(0, 0, image.cols / 2, image.rows); //��벿��
			Rect rectRight(image.cols / 2, 0, image.cols / 2, image.rows);//�Ұ벿�� 

			Mat imageLeft = image(rectLeft);
			Mat imageRight = image(rectRight);

			string new_left_name = "E:/second_data/calibrate/1/left/" + name + "_left.jpg";
			string new_right_name = "E:/second_data/calibrate/1/right/" + name + "_right.jpg";

			imwrite(new_left_name, imageLeft);
			imwrite(new_right_name, imageRight);

			outLeft << new_left_name << endl;
			outRight << new_right_name << endl;
		}
	}

	string calibrate_result = "E:/second_data/calibrate/1/double_calib_result.xml";
	int cornerNumInWidth = 9;
	int cornerNumInHeight = 6;
	int lengthOfSquare = 26;
	DoubleCalibrate calib(leftImagePath, rightImagePath, cornerNumInWidth, cornerNumInHeight, lengthOfSquare, calibrate_result);

	//��ά�ǵ�ʵ������
	string left_image_rectify = "E:/second_data/calibrate/1/left/calibrate_10570_left.jpg";
	string right_image_rectify = "E:/second_data/calibrate/1/right/calibrate_10570_right.jpg";
	calib.ProcessCalibrate();
	calib.CalcTransformMap();
	calib.ShowRectified(left_image_rectify, right_image_rectify);
}



/// <summary>
/// �����𽥽ӽ�����ͷ��̽����ͬIntervalʱ�����ɵľ��ο�������ܹ���ʱ
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int car_avoid_yoloV4_KCF_intervalX(int argc, char* argv[])
{
	if (argc > 5) return -1;

	bool HOG = true;
	bool FIXEDWINDOW = false;
	bool MULTISCALE = true;
	bool SILENT = true;
	bool LAB = false;

	// �������ã��ɲ��ù�
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "hog") == 0)
			HOG = true;
		if (strcmp(argv[i], "fixed_window") == 0)
			FIXEDWINDOW = true;
		if (strcmp(argv[i], "singlescale") == 0)
			MULTISCALE = false;
		if (strcmp(argv[i], "show") == 0)
			SILENT = false;
		if (strcmp(argv[i], "lab") == 0) {
			LAB = true;
			HOG = true;
		}
		if (strcmp(argv[i], "gray") == 0)
			HOG = false;
	}

	// KCFĿ�������
	KCFTracker trackerLeft(HOG, FIXEDWINDOW, MULTISCALE, LAB);
	KCFTracker trackerRight(HOG, FIXEDWINDOW, MULTISCALE, LAB);

	// YOLOĿ������
	string cfg = "../darknet/cfg/yolov4.cfg";
	string weight = "../darknet/cfg/yolov4.weights";
	Detector carDetector = Detector(cfg, weight, 0);

	// �궨����׼��
	string leftImagePath = "D:/dataset/calibrateImage/select/left/left.txt";
	string rightImagePath = "D:/dataset/calibrateImage/select/right/right.txt";
	string calibrate_result = "D:/dataset/calibrateImage/select/double_calib_result.xml";
	int cornerNumInWidth = 9;
	int cornerNumInHeight = 6;
	int lengthOfSquare = 26;
	DoubleCalibrate calib(leftImagePath, rightImagePath, cornerNumInWidth, cornerNumInHeight, lengthOfSquare, calibrate_result);
	calib.ProcessCalibrate();
	calib.CalcTransformMap();

	// ���ò�ͬ��YOLO�����������Ŀ��λ�úʹ����ٶ�
	// YOLO�����Ϊ2��YKYKYK
	// YOLO�����Ϊ3��YKKYKKYKK
	for (int interval = 1; interval <= 100; interval++) {
		cout << "interval: " << interval << "\r";
		std::ofstream outLeft, outRight;
		outLeft.open("E:/second_data/interval/" + to_string(interval) + "_rect_left.txt");
		outRight.open("E:/second_data/interval/" + to_string(interval) + "_rect_right.txt");

		// ��ԭʼ��Ƶ�ļ�
		VideoCapture cap("E:/second_data/60_use.mp4");

		// ���洦������Ƶ
		int fps = 30;
		Size size = Size(1280, 720);
		VideoWriter writerLeft("../data/car_post_process_left_" + to_string(interval) + "_use_speedX.mp4", VideoWriter::fourcc('D', 'I', 'V', 'X'), fps, size, true);
		VideoWriter writerRight("../data/car_post_process_right_" + to_string(interval) + "_use_speedX.mp4", VideoWriter::fourcc('D', 'I', 'V', 'X'), fps, size, true);
		VideoWriter writerCombo("../data/car_post_process_combo_" + to_string(interval) + "_use_speedX.mp4", VideoWriter::fourcc('D', 'I', 'V', 'X'), fps, cv::Size(1280, 360), true);

		oak::Timer calcTime;
		calcTime.Start();

		Mat image, imageLeft, imageRight;

		// �Ƿ�����YOLO���
		bool isDetect = true;
		int count = 0;

		while (1)
		{
			// ��ȡһ֡ͼ��
			cap >> image;
			if (image.empty()) break;

			// ��������ͷͼ����
			Rect rectLeft(0, 0, image.cols / 2, image.rows); //��벿��
			Rect rectRight(image.cols / 2, 0, image.cols / 2, image.rows);//�Ұ벿�� 

			imageLeft = image(rectLeft);
			imageRight = image(rectRight);

			cv::Mat imageLeftRectify;
			cv::Mat imageRightRectify;
			cv::Mat combo;

			//����ͼƬ
			calib.ShowRectified(imageLeft, imageRight, imageLeftRectify, imageRightRectify);

			++count;

			if (isDetect == true) {
				// ʹ��YOLO��⵽׼ȷ��Ŀ��λ��
				std::vector<bbox_t> posLeft = carDetector.detect(imageLeftRectify);
				std::vector<bbox_t> posRight = carDetector.detect(imageRightRectify);
				if (posLeft.size() <= 0 || posRight.size() <= 0) {
					continue;
				}

				// �ҵ����������ο�
				bbox_t maxLeft = get_max_one(posLeft);
				bbox_t maxRight = get_max_one(posRight);

				// �ҵ���ɫ����
				//bbox_t maxLeft = get_red_one(imageLeftRectify, posLeft);
				//bbox_t maxRight = get_red_one(imageRightRectify, posRight);

				isDetect = false;
				// �þ�ȷλ�ó�ʼ��KCF
				trackerLeft.init(Rect(maxLeft.x, maxLeft.y, maxLeft.w, maxLeft.h), imageLeftRectify);
				trackerRight.init(Rect(maxRight.x, maxRight.y, maxRight.w, maxRight.h), imageRightRectify);

				//cv::rectangle(imageLeftRectify, Point(maxLeft.x, maxLeft.y), Point(maxLeft.x + maxLeft.w, maxLeft.y + maxLeft.h), Scalar(0, 0, 255), 3, 8, 0);
				//cv::rectangle(imageRightRectify, Point(maxRight.x, maxRight.y), Point(maxRight.x + maxRight.w, maxRight.y + maxRight.h), Scalar(0, 0, 255), 3, 8, 0);

				outLeft << "initLeft :" << count << ", " << maxLeft.x << ", " << maxLeft.y << ", " << maxLeft.w << ", " << maxLeft.h << endl;
				cout << "initLeft :" << count << ", " << maxLeft.x << ", " << maxLeft.y << ", " << maxLeft.w << ", " << maxLeft.h << endl;

				outRight << "initRight :" << count << ", " << maxRight.x << ", " << maxRight.y << ", " << maxRight.w << ", " << maxRight.h << endl;
				cout << "initRight :" << count << ", " << maxRight.x << ", " << maxRight.y << ", " << maxRight.w << ", " << maxRight.h << endl;

				//cv::circle(imageLeftRectify, cv::Point(maxLeft.x + maxLeft.w / 2, maxLeft.y + maxLeft.h / 2), 5, cv::Scalar(0, 255, 0), -1);
				//cv::circle(imageRightRectify, cv::Point(maxRight.x + maxRight.w / 2, maxRight.y + maxRight.h / 2), 5, cv::Scalar(0, 255, 0), -1);

				// double distance = 562 * 12 / 3.75f / abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2));
				// double distance = 525 * 3.035 / abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2));
				// cout << abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2)) << "*****************distance: " << distance << endl;
				//cout << "distance: " << distance << endl;
				//outLeft << "distance: " << distance << endl;

				//string text = "distance: " + to_string(distance);
				//cv::Point origin(100, 100);
				//cv::putText(imageLeftRectify, text, origin, cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0, 255, 0), 2, 8, 0);
			}
			else {
				// ��ʹ��KCF����Ŀ��λ��
				cv::Rect posLeft = trackerLeft.update(imageLeftRectify);
				cv::Rect posRight = trackerRight.update(imageRightRectify);

				//cv::rectangle(imageLeftRectify, Point(posLeft.x, posLeft.y), Point(posLeft.x + posLeft.width, posLeft.y + posLeft.height), Scalar(0, 0, 255), 2, 8);
				//cv::rectangle(imageRightRectify, Point(posRight.x, posRight.y), Point(posRight.x + posRight.width, posRight.y + posRight.height), Scalar(0, 0, 255), 2, 8);

				outLeft << "trackLeft :" << count << ", " << posLeft.x << ", " << posLeft.y << ", " << posLeft.width << ", " << posLeft.height << endl;
				cout << "trackLeft :" << count << ", " << posLeft.x << ", " << posLeft.y << ", " << posLeft.width << ", " << posLeft.height << endl;

				outRight << "trackRight :" << count << ", " << posRight.x << ", " << posRight.y << ", " << posRight.width << ", " << posRight.height << endl;
				cout << "trackRight :" << count << ", " << posRight.x << ", " << posRight.y << ", " << posRight.width << ", " << posRight.height << endl;
			}

			if (count % interval == 0) {
				isDetect = true;
			}

			////������ͼ��ƴ����һ��
			//combo = Mat(imageLeftRectify.rows, 2 * imageLeftRectify.cols, CV_8UC3);
			//imageLeftRectify.copyTo(combo(rectLeft));// frame_left_rect��������벿��
			//imageRightRectify.copyTo(combo(rectRight));// frame_right_rect�������Ұ벿��

			//// Draw horizontal red lines in the combo image to make comparison easier
			//for (int y = 0; y < combo.rows; y += 20)
			//{
			//	line(combo, Point(0, y), Point(combo.cols, y), Scalar(0, 0, 255));
			//}

			//resize(combo, combo, cv::Size(combo.cols / 2, combo.rows / 2));
			//imshow("Combo", combo);
			//waitKey(1);

			//imshow("imageLeft", imageLeftRectify);
			//imshow("imageRight", imageRightRectify);
			//waitKey(1);
			//writerLeft.write(imageLeftRectify);
			//writerRight.write(imageRightRectify);
			//writerCombo.write(combo);
		}

		//writerLeft.release();
		//writerRight.release();
		//writerCombo.release();
		calcTime.Stop();
		double elapseTime = calcTime.GetElapsedMilliseconds() / 1000;

		std::cout << "elapseTime:" << elapseTime << std::endl;
		outLeft << "elapseTime: " << elapseTime << std::endl;
		outLeft.close();

		outRight << "elapseTime: " << elapseTime << std::endl;
		outRight.close();
	}
}



/// <summary>
/// ��ͬ�̶���λʱ������ÿ����Ƶÿ֡ͼ��Ĳ�������
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int car_avoid_yoloV4_generate_distance(int argc, char* argv[])
{
	if (argc > 5) return -1;

	bool HOG = true;
	bool FIXEDWINDOW = false;
	bool MULTISCALE = true;
	bool SILENT = true;
	bool LAB = false;

	// �������ã��ɲ��ù�
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "hog") == 0)
			HOG = true;
		if (strcmp(argv[i], "fixed_window") == 0)
			FIXEDWINDOW = true;
		if (strcmp(argv[i], "singlescale") == 0)
			MULTISCALE = false;
		if (strcmp(argv[i], "show") == 0)
			SILENT = false;
		if (strcmp(argv[i], "lab") == 0) {
			LAB = true;
			HOG = true;
		}
		if (strcmp(argv[i], "gray") == 0)
			HOG = false;
	}

	// KCFĿ�������
	KCFTracker trackerLeft(HOG, FIXEDWINDOW, MULTISCALE, LAB);
	KCFTracker trackerRight(HOG, FIXEDWINDOW, MULTISCALE, LAB);

	// YOLOĿ������
	string cfg = "../darknet/cfg/yolov4.cfg";
	string weight = "../darknet/cfg/yolov4.weights";
	Detector carDetector = Detector(cfg, weight, 0);

	// �궨����׼��
	string leftImagePath = "D:/dataset/calibrateImage/select/left/left.txt";
	string rightImagePath = "D:/dataset/calibrateImage/select/right/right.txt";
	string calibrate_result = "D:/dataset/calibrateImage/select/double_calib_result.xml";
	//string leftImagePath = "E:/second_data/calibrate/1/left/left.txt";
	//string rightImagePath = "E:/second_data/calibrate/1/right/right.txt";
	//string calibrate_result = "E:/second_data/calibrate/1/double_calib_result.xml";
	int cornerNumInWidth = 9;
	int cornerNumInHeight = 6;
	int lengthOfSquare = 26;
	DoubleCalibrate calib(leftImagePath, rightImagePath, cornerNumInWidth, cornerNumInHeight, lengthOfSquare, calibrate_result);
	calib.ProcessCalibrate();
	calib.CalcTransformMap();

	// ���ò�ͬ��YOLO�����������Ŀ��λ�úʹ����ٶ�
	// YOLO�����Ϊ2��YKYKYK
	// YOLO�����Ϊ3��YKKYKKYKK
	int interval = 30; // ���ż����
	double measure = 0;
	for (int distance = 20; distance <= 50; distance = distance + 10) {
		std::ofstream out;
		out.open("E:/second_data/" + to_string(distance) + "_use_distance.txt");

		// ��ԭʼ��Ƶ�ļ�
		VideoCapture cap("E:/second_data/" + to_string(distance) + "_use.mp4");

		// ���洦������Ƶ
		int fps = 30;
		Size size = Size(1280, 720);
		VideoWriter writerLeft("../data/car_post_process_left_" + to_string(distance) + "_use.mp4", VideoWriter::fourcc('D', 'I', 'V', 'X'), fps, size, true);
		VideoWriter writerRight("../data/car_post_process_right_" + to_string(distance) + "_use.mp4", VideoWriter::fourcc('D', 'I', 'V', 'X'), fps, size, true);
		VideoWriter writerCombo("../data/car_post_process_combo_" + to_string(distance) + "_use.mp4", VideoWriter::fourcc('D', 'I', 'V', 'X'), fps, cv::Size(1280, 360), true);

		oak::Timer calcTime;
		calcTime.Start();

		Mat image, imageLeft, imageRight;
		bool isDetect = true;// �Ƿ�����YOLO���
		int count = 0;

		while (1)
		{
			// ��ȡһ֡ͼ��
			cap >> image;
			if (image.empty()) break;

			// ��������ͷͼ����
			Rect rectLeft(0, 0, image.cols / 2, image.rows); //��벿��
			Rect rectRight(image.cols / 2, 0, image.cols / 2, image.rows);//�Ұ벿�� 

			imageLeft = image(rectLeft);
			imageRight = image(rectRight);
			cv::Mat imageLeftRectify;
			cv::Mat imageRightRectify;
			cv::Mat combo;
			//����ͼ��
			calib.ShowRectified(imageLeft, imageRight, imageLeftRectify, imageRightRectify);

			++count;

			if (isDetect == true) {
				// ʹ��YOLO��⵽׼ȷ��Ŀ��λ��
				std::vector<bbox_t> posLeft = carDetector.detect(imageLeftRectify);
				std::vector<bbox_t> posRight = carDetector.detect(imageRightRectify);
				if (posLeft.size() <= 0 || posRight.size() <= 0) {
					continue;
				}

				//�ҳ����������ο�
				bbox_t maxLeft = get_max_one(posLeft);
				bbox_t maxRight = get_max_one(posRight);

				//�ҳ���ɫ����
				//bbox_t maxLeft = get_red_one(imageLeftRectify, posLeft);
				//bbox_t maxRight = get_red_one(imageRightRectify, posRight);

				isDetect = false;
				// �þ�ȷλ�ó�ʼ��KCF
				trackerLeft.init(Rect(maxLeft.x, maxLeft.y, maxLeft.w, maxLeft.h), imageLeftRectify);
				trackerRight.init(Rect(maxRight.x, maxRight.y, maxRight.w, maxRight.h), imageRightRectify);

				cv::rectangle(imageLeftRectify, Point(maxLeft.x, maxLeft.y), Point(maxLeft.x + maxLeft.w, maxLeft.y + maxLeft.h), Scalar(0, 0, 255), 3, 8, 0);
				cv::rectangle(imageRightRectify, Point(maxRight.x, maxRight.y), Point(maxRight.x + maxRight.w, maxRight.y + maxRight.h), Scalar(0, 0, 255), 3, 8, 0);

				cout << "initLeft :" << count << ", " << maxLeft.x << ", " << maxLeft.y << ", " << maxLeft.w << ", " << maxLeft.h << endl;
				cout << "initRight :" << count << ", " << maxRight.x << ", " << maxRight.y << ", " << maxRight.w << ", " << maxRight.h << endl;

				cv::circle(imageLeftRectify, cv::Point(maxLeft.x + maxLeft.w / 2, maxLeft.y + maxLeft.h / 2), 5, cv::Scalar(0, 255, 0), -1);
				cv::circle(imageRightRectify, cv::Point(maxRight.x + maxRight.w / 2, maxRight.y + maxRight.h / 2), 5, cv::Scalar(0, 255, 0), -1);

				measure = 562 * 12 / 3.75f / abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2));
				// double measure = 525 * 3.035 / abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2));
				cout << "diff:" << abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2)) << ", distance: " << measure << endl;
				out << "diff:" << abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2)) << ", distance: " << measure;
				out << " ,initLeft :" << count << ", " << maxLeft.x << ", " << maxLeft.y << ", " << maxLeft.w << ", " << maxLeft.h;
				out << " ,initRight :" << count << ", " << maxRight.x << ", " << maxRight.y << ", " << maxRight.w << ", " << maxRight.h << endl;

				string text = "diff: " + to_string(abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2))) + ", distance: " + to_string(measure);
				cv::Point origin(800, 700);
				cv::putText(imageLeftRectify, text, origin, cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, 8, 0);
			}
			else {
				// ��ʹ��KCF����Ŀ��λ��
				cv::Rect posLeft = trackerLeft.update(imageLeftRectify);
				cv::Rect posRight = trackerRight.update(imageRightRectify);

				cv::rectangle(imageLeftRectify, Point(posLeft.x, posLeft.y), Point(posLeft.x + posLeft.width, posLeft.y + posLeft.height), Scalar(0, 0, 255), 2, 8);
				cv::rectangle(imageRightRectify, Point(posRight.x, posRight.y), Point(posRight.x + posRight.width, posRight.y + posRight.height), Scalar(0, 0, 255), 2, 8);

				cout << "trackLeft :" << count << ", " << posLeft.x << ", " << posLeft.y << ", " << posLeft.width << ", " << posLeft.height << endl;
				cout << "trackRight :" << count << ", " << posRight.x << ", " << posRight.y << ", " << posRight.width << ", " << posRight.height << endl;

				cv::circle(imageLeftRectify, cv::Point(posLeft.x + posLeft.width / 2, posLeft.y + posLeft.height / 2), 5, cv::Scalar(0, 255, 0), -1);
				cv::circle(imageRightRectify, cv::Point(posRight.x + posRight.width / 2, posRight.y + posRight.height / 2), 5, cv::Scalar(0, 255, 0), -1);

				measure = 562 * 12 / 3.75f / abs(int(posLeft.x) + int(posLeft.width / 2) - int(posRight.x) - int(posRight.width / 2));
				// double measure = 525 * 3.035 / abs(int(posLeft.x) + int(posLeft.width / 2) - int(posRight.x) - int(posRight.width / 2));
				cout << "diff:" << abs(int(posLeft.x) + int(posLeft.width / 2) - int(posRight.x) - int(posRight.width / 2)) << ", distance: " << measure << endl;
				out << "diff:" << abs(int(posLeft.x) + int(posLeft.width / 2) - int(posRight.x) - int(posRight.width / 2)) << ", distance: " << measure;
				out << " ,trackLeft :" << count << ", " << posLeft.x << ", " << posLeft.y << ", " << posLeft.width << ", " << posLeft.height;
				out << " ,trackRight :" << count << ", " << posRight.x << ", " << posRight.y << ", " << posRight.width << ", " << posRight.height << endl;

				string text = "diff: " + to_string(abs(int(posLeft.x) + int(posLeft.width / 2) - int(posRight.x) - int(posRight.width / 2))) + ", distance: " + to_string(measure);
				cv::Point origin(800, 700);
				cv::putText(imageLeftRectify, text, origin, cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, 8, 0);
			}

			if (count % interval == 0) {
				isDetect = true;
			}
			isDetect = true;// �������ʱ��ǿ��ʹ��yolo���

			//������ͼ��ƴ����һ��
			combo = Mat(imageLeftRectify.rows, 2 * imageLeftRectify.cols, CV_8UC3);
			imageLeftRectify.copyTo(combo(rectLeft));// frame_left_rect��������벿��
			imageRightRectify.copyTo(combo(rectRight));// frame_right_rect�������Ұ벿��

			// Draw horizontal red lines in the combo image to make comparison easier
			for (int y = 0; y < combo.rows; y += 20)
			{
				line(combo, Point(0, y), Point(combo.cols, y), Scalar(0, 0, 255));
			}

			resize(combo, combo, cv::Size(combo.cols / 2, combo.rows / 2));
			imshow("Combo", combo);
			waitKey(1);

			imshow("imageLeft", imageLeftRectify);
			imshow("imageRight", imageRightRectify);
			waitKey(1);

			writerLeft.write(imageLeftRectify);
			writerRight.write(imageRightRectify);
			writerCombo.write(combo);
		}

		writerLeft.release();
		writerRight.release();
		writerCombo.release();
		calcTime.Stop();
		double elapseTime = calcTime.GetElapsedMilliseconds() / 1000;

		std::cout << "elapseTime:" << elapseTime << std::endl;
		out << "elapseTime: " << elapseTime << std::endl;
		out.close();
	}
}



/// <summary>
/// ���ݲ�ͬ�̶���λ�õ��Ĳ������룬����TPR��FPR���ó�����ʵ�Ԥ������
/// </summary>
/// <returns></returns>
int car_avoid_alarm_distance()
{
	string temp;
	for (int alarm = 20; alarm <= 50; alarm = alarm + 10) {
		int TP = 0;
		int FN = 0;
		float TPR = 0;
		int TN = 0;
		int FP = 0;
		float FPR = 0;

		for (int distance = 20; distance <= 50; distance = distance + 10) {
			ifstream file("E:/second_data/" + to_string(distance) + "_use_distance.txt");
			while (getline(file, temp)) {
				//cout << "temp:" << temp << endl;
				std::string::size_type pos = temp.find("distance:");

				if (pos != std::string::npos) {
					temp = temp.substr(pos + 10, 4);
					float measure = atof(temp.c_str());
					//cout << "dis:" << temp << ", " << measure << endl;
					if (distance <= alarm && measure <= alarm) {
						//cout << "TP, alarm:" << alarm << ", distance:" << distance << ", measure:" << measure << endl;
						TP++;
					}
					else if (distance <= alarm && measure > alarm) {
						//cout << "FN, alarm:" << alarm << ", distance:" << distance << ", measure:" << measure << endl;
						FN++;
					}
					else if (distance > alarm && measure > alarm) {
						//cout << "TN, alarm:" << alarm << ", distance:" << distance << ", measure:" << measure << endl;
						TN++;
					}
					else if (distance > alarm && measure <= alarm) {
						//cout << "FP, alarm:" << alarm << ", distance:" << distance << ", measure:" << measure << endl;
						FP++;
					}
				}
			}
		}

		TPR = float(TP) / (float(TP) + float(FN));
		FPR = float(FP) / (float(FP) + float(TN));
		cout << "alarm:" << alarm << ", TP:" << TP << ", FN:" << FN << ", TPR:" << TPR << ", FP:" << FP << ", TN:" << TN << ", FPR:" << FPR << endl;
	}
}



/// <summary>
/// ��Ѽ��30��Ԥ������20m��ͬʱ�����ڵ�����Ƶ��Ч��
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int car_avoid_yoloV4_final(int argc, char* argv[])
{
	if (argc > 5) return -1;

	bool HOG = true;
	bool FIXEDWINDOW = false;
	bool MULTISCALE = true;
	bool SILENT = true;
	bool LAB = false;

	// �������ã��ɲ��ù�
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "hog") == 0)
			HOG = true;
		if (strcmp(argv[i], "fixed_window") == 0)
			FIXEDWINDOW = true;
		if (strcmp(argv[i], "singlescale") == 0)
			MULTISCALE = false;
		if (strcmp(argv[i], "show") == 0)
			SILENT = false;
		if (strcmp(argv[i], "lab") == 0) {
			LAB = true;
			HOG = true;
		}
		if (strcmp(argv[i], "gray") == 0)
			HOG = false;
	}

	// KCFĿ�������
	KCFTracker trackerLeft(HOG, FIXEDWINDOW, MULTISCALE, LAB);
	KCFTracker trackerRight(HOG, FIXEDWINDOW, MULTISCALE, LAB);

	// YOLOĿ������
	string cfg = "../darknet/cfg/yolov4.cfg";
	string weight = "../darknet/cfg/yolov4.weights";
	Detector carDetector = Detector(cfg, weight, 0);

	// �궨����׼��
	string leftImagePath = "D:/dataset/calibrateImage/select/left/left.txt";
	string rightImagePath = "D:/dataset/calibrateImage/select/right/right.txt";
	string calibrate_result = "D:/dataset/calibrateImage/select/double_calib_result.xml";
	int cornerNumInWidth = 9;
	int cornerNumInHeight = 6;
	int lengthOfSquare = 26;
	DoubleCalibrate calib(leftImagePath, rightImagePath, cornerNumInWidth, cornerNumInHeight, lengthOfSquare, calibrate_result);
	calib.ProcessCalibrate();
	calib.CalcTransformMap();

	// ���ò�ͬ��YOLO�����������Ŀ��λ�úʹ����ٶ�
	// YOLO�����Ϊ2��YKYKYK
	// YOLO�����Ϊ3��YKKYKKYKK
	int interval = 30; // ���ż����
	double measure = 0;
	for (int distance = 60; distance <= 60; distance = distance + 10) {
		std::ofstream out;
		out.open("E:/second_data/" + to_string(distance) + "_use_distance.txt");

		// ��ԭʼ��Ƶ�ļ�
		VideoCapture cap("E:/second_data/" + to_string(distance) + "_use.mp4");

		// ���洦������Ƶ
		int fps = 30;
		Size size = Size(1280, 720);
		VideoWriter writerLeft("../data/car_post_process_left_" + to_string(distance) + "_use_final.mp4", VideoWriter::fourcc('D', 'I', 'V', 'X'), fps, size, true);
		VideoWriter writerRight("../data/car_post_process_right_" + to_string(distance) + "_use_final.mp4", VideoWriter::fourcc('D', 'I', 'V', 'X'), fps, size, true);
		VideoWriter writerCombo("../data/car_post_process_combo_" + to_string(distance) + "_use_final.mp4", VideoWriter::fourcc('D', 'I', 'V', 'X'), fps, cv::Size(1280, 360), true);

		oak::Timer calcTime;
		calcTime.Start();

		Mat image, imageLeft, imageRight;
		bool isDetect = true;// �Ƿ�����YOLO���
		int count = 0;

		while (1)
		{
			// ��ȡһ֡ͼ��
			cap >> image;
			if (image.empty()) break;

			// ��������ͷͼ����
			Rect rectLeft(0, 0, image.cols / 2, image.rows); //��벿��
			Rect rectRight(image.cols / 2, 0, image.cols / 2, image.rows);//�Ұ벿�� 

			imageLeft = image(rectLeft);
			imageRight = image(rectRight);
			cv::Mat imageLeftRectify;
			cv::Mat imageRightRectify;
			cv::Mat combo;
			//����ͼ��
			calib.ShowRectified(imageLeft, imageRight, imageLeftRectify, imageRightRectify);

			++count;

			if (isDetect == true) {
				// ʹ��YOLO��⵽׼ȷ��Ŀ��λ��
				std::vector<bbox_t> posLeft = carDetector.detect(imageLeftRectify);
				std::vector<bbox_t> posRight = carDetector.detect(imageRightRectify);
				if (posLeft.size() <= 0 || posRight.size() <= 0) {
					continue;
				}

				bbox_t maxLeft = get_max_one(posLeft);
				bbox_t maxRight = get_max_one(posRight);
				//bbox_t maxLeft = get_red_one(imageLeftRectify, posLeft);
				//bbox_t maxRight = get_red_one(imageRightRectify, posRight);

				isDetect = false;
				// �þ�ȷλ�ó�ʼ��KCF
				trackerLeft.init(Rect(maxLeft.x, maxLeft.y, maxLeft.w, maxLeft.h), imageLeftRectify);
				trackerRight.init(Rect(maxRight.x, maxRight.y, maxRight.w, maxRight.h), imageRightRectify);

				cv::rectangle(imageLeftRectify, Point(maxLeft.x, maxLeft.y), Point(maxLeft.x + maxLeft.w, maxLeft.y + maxLeft.h), Scalar(0, 0, 255), 3, 8, 0);
				cv::rectangle(imageRightRectify, Point(maxRight.x, maxRight.y), Point(maxRight.x + maxRight.w, maxRight.y + maxRight.h), Scalar(0, 0, 255), 3, 8, 0);

				cout << "initLeft :" << count << ", " << maxLeft.x << ", " << maxLeft.y << ", " << maxLeft.w << ", " << maxLeft.h << endl;
				cout << "initRight :" << count << ", " << maxRight.x << ", " << maxRight.y << ", " << maxRight.w << ", " << maxRight.h << endl;

				cv::circle(imageLeftRectify, cv::Point(maxLeft.x + maxLeft.w / 2, maxLeft.y + maxLeft.h / 2), 5, cv::Scalar(0, 255, 0), -1);
				cv::circle(imageRightRectify, cv::Point(maxRight.x + maxRight.w / 2, maxRight.y + maxRight.h / 2), 5, cv::Scalar(0, 255, 0), -1);

				measure = 562 * 12 / 3.75f / abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2));
				// double measure = 525 * 3.035 / abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2));
				cout << "diff:" << abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2)) << ", distance: " << measure << endl;
				out << "diff:" << abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2)) << ", distance: " << measure;
				out << ", initLeft :" << count << ", " << maxLeft.x << ", " << maxLeft.y << ", " << maxLeft.w << ", " << maxLeft.h;
				out << ", initRight :" << count << ", " << maxRight.x << ", " << maxRight.y << ", " << maxRight.w << ", " << maxRight.h << endl;

				string text = "diff: " + to_string(abs(int(maxLeft.x) + int(maxLeft.w / 2) - int(maxRight.x) - int(maxRight.w / 2))) + ", distance: " + to_string(measure);
				cv::Point origin(800, 700);
				cv::putText(imageLeftRectify, text, origin, cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, 8, 0);
			}
			else {
				// ��ʹ��KCF����Ŀ��λ��
				cv::Rect posLeft = trackerLeft.update(imageLeftRectify);
				cv::Rect posRight = trackerRight.update(imageRightRectify);

				cv::rectangle(imageLeftRectify, Point(posLeft.x, posLeft.y), Point(posLeft.x + posLeft.width, posLeft.y + posLeft.height), Scalar(0, 0, 255), 2, 8);
				cv::rectangle(imageRightRectify, Point(posRight.x, posRight.y), Point(posRight.x + posRight.width, posRight.y + posRight.height), Scalar(0, 0, 255), 2, 8);

				cout << "trackLeft :" << count << ", " << posLeft.x << ", " << posLeft.y << ", " << posLeft.width << ", " << posLeft.height << endl;
				cout << "trackRight :" << count << ", " << posRight.x << ", " << posRight.y << ", " << posRight.width << ", " << posRight.height << endl;

				cv::circle(imageLeftRectify, cv::Point(posLeft.x + posLeft.width / 2, posLeft.y + posLeft.height / 2), 5, cv::Scalar(0, 255, 0), -1);
				cv::circle(imageRightRectify, cv::Point(posRight.x + posRight.width / 2, posRight.y + posRight.height / 2), 5, cv::Scalar(0, 255, 0), -1);

				measure = 562 * 12 / 3.75f / abs(int(posLeft.x) + int(posLeft.width / 2) - int(posRight.x) - int(posRight.width / 2));
				// double measure = 525 * 3.035 / abs(int(posLeft.x) + int(posLeft.width / 2) - int(posRight.x) - int(posRight.width / 2));
				cout << "diff:" << abs(int(posLeft.x) + int(posLeft.width / 2) - int(posRight.x) - int(posRight.width / 2)) << ", distance: " << measure << endl;
				out << "diff:" << abs(int(posLeft.x) + int(posLeft.width / 2) - int(posRight.x) - int(posRight.width / 2)) << ", distance: " << measure;
				out << ", trackLeft :" << count << ", " << posLeft.x << ", " << posLeft.y << ", " << posLeft.width << ", " << posLeft.height;
				out << ", trackRight :" << count << ", " << posRight.x << ", " << posRight.y << ", " << posRight.width << ", " << posRight.height << endl;

				string text = "diff: " + to_string(abs(int(posLeft.x) + int(posLeft.width / 2) - int(posRight.x) - int(posRight.width / 2))) + ", distance: " + to_string(measure);
				cv::Point origin(800, 700);
				cv::putText(imageLeftRectify, text, origin, cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, 8, 0);
			}

			if (count % interval == 0) {
				isDetect = true;
			}
			// isDetect = true;// ǿ��ʹ��yolo���

			if (measure < 20) {
				string text = "alarm!!!";
				cv::Point origin(800, 600);
				cv::putText(imageLeftRectify, text, origin, cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, 8, 0);
			}

			//������ͼ��ƴ����һ��
			combo = Mat(imageLeftRectify.rows, 2 * imageLeftRectify.cols, CV_8UC3);
			imageLeftRectify.copyTo(combo(rectLeft));// frame_left_rect��������벿��
			imageRightRectify.copyTo(combo(rectRight));// frame_right_rect�������Ұ벿��

			// Draw horizontal red lines in the combo image to make comparison easier
			for (int y = 0; y < combo.rows; y += 20)
			{
				line(combo, Point(0, y), Point(combo.cols, y), Scalar(0, 0, 255));
			}

			resize(combo, combo, cv::Size(combo.cols / 2, combo.rows / 2));
			imshow("Combo", combo);
			waitKey(1);

			imshow("imageLeft", imageLeftRectify);
			imshow("imageRight", imageRightRectify);
			waitKey(1);

			writerLeft.write(imageLeftRectify);
			writerRight.write(imageRightRectify);
			writerCombo.write(combo);
		}

		writerLeft.release();
		writerRight.release();
		writerCombo.release();
		calcTime.Stop();
		double elapseTime = calcTime.GetElapsedMilliseconds() / 1000;

		std::cout << "elapseTime:" << elapseTime << std::endl;
		out << "elapseTime: " << elapseTime << std::endl;
		out.close();
	}
}



int main(int argc, char* argv[]){
	//generateCalibrateImages();
	//splitCalibrateImages();
	//car_avoid_yoloV4(argc, argv);
	//generateIOU_time_right();
	
	//car_avoid_yoloV4_KCF_intervalX(argc, argv);
	//car_avoid_yoloV4_generate_distance(argc, argv);
	car_avoid_alarm_distance();
	//car_avoid_yoloV4_final(argc, argv);

	system("pause");
	return 0;
}