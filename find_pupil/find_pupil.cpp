#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
#include "boost/algorithm/string.hpp"
//#define MAX_FRAMES 20000
#define MAX_FRAMES 200 * 60 * 60

using namespace cv;
using namespace std;

bool get_iris = false;
Mat src, erosion_dst, dilation_dst; Mat src_gray;
int thresh = 40;
int max_thresh = 60;
int iris_thresh = 60;
int max_iris_thresh = 150;
RNG rng(12345);
int i_x;
float pupil_width[MAX_FRAMES],pupil_height[MAX_FRAMES],pupil_width2[MAX_FRAMES],pupil_height2[MAX_FRAMES];

int from = 0;
int to = 0;
int downArea = 0;
int upArea = 0;
int erosion_elem = 0;
int erosion_size = 0;
int const max_elem = 2;
int const max_kernel_size = 21;

ostringstream obuff;
ifstream ifs;
ofstream ofs;
string str;
FILE *fp;
vector<vector<std::string> > array( MAX_FRAMES, vector<std::string>(2) );
RotatedRect minEllipse;
RotatedRect minEllipse2;
/// Function header
void thresh_callback(int, void* );
void thresh_callback2(int, void* );

void Erosion(int, void*);
/** @function main */
int main( int argc, char** argv )
{
  char fname[256],ofnamecsv[256],wfnamecsv[256];

  printf("何フレームから:");
  cin >> from;
  printf("何フレームまで:");
  cin >> to;
  printf("面積下限:");
  cin >> downArea;
  printf("面積上限:");
  cin >> upArea;
  printf("\n");

  for(i_x=from; i_x < to; i_x++ ){

	  /// create filename
	  sprintf_s(fname,"%s\\%04d.png",argv[1], i_x);
	  printf("file = %s\n", fname);

	  /// Load source image and convert it to gray
	  src = imread( fname, 1 );
	  if( src.data == NULL ){
		  printf("cannot read file %s\n", fname);
		  Sleep(1000);
		  break;
		  //goto LOOPEND;
	  }

	  /// Convert image to gray and blur it
	  cvtColor( src, src_gray, CV_BGR2GRAY );
	  blur( src_gray, src_gray, Size(3,3) );

	  /// Create Window
	  char* source_window = "Source";
	  namedWindow( source_window, CV_WINDOW_AUTOSIZE);
	  //imshow(source_window, src_gray);

	  createTrackbar( " Threshold:", "Source", &thresh, max_thresh, thresh_callback );
	  if(get_iris){
		createTrackbar( " Threshold2:", "Source", &iris_thresh, max_iris_thresh, thresh_callback2 );
		thresh_callback2( 0, 0 );
	  }

	  /// Create Erosion Trackbar
	  createTrackbar("Element:\n 0: Rect \n 1: Cross \n 2: Ellipse", "Source",
		  &erosion_elem, max_elem,
		  Erosion);

	  createTrackbar("Kernel size:\n 2n +1", "Source",
		  &erosion_size, max_kernel_size,
		  Erosion);

	  Erosion(0, 0);
	  thresh_callback(0, 0);

	  //imshow(source_window, src_gray);

	  LOOPEND:
	  if( i_x == from ){
		  int key = waitKey(0);

		  if( key == 0x1b ){
			  break;
		  }
	  } else {
		cvWaitKey(10);
	  }
  }
  //make the timing file 
  
	sprintf_s(ofnamecsv,"%s\\timex.csv",argv[1]);
	ifs.open(ofnamecsv);
	int n=0;
	while(ifs && getline(ifs, str)) {
		boost::algorithm::split(array[n], str, boost::is_any_of(","));
		n++;
	}
	ifs.close();
	
	sprintf_s(wfnamecsv,"%s\\outputdata.csv",argv[1]);
	ofs.open(wfnamecsv);

	for(int i=from;i<to;i++){ 
		char buff[8];
		string strbuff;
		sprintf_s(buff,"%d",i);
		for(int o=0;o<n;o++){
			if(array[o][0] == buff){
				strbuff = array[o][1];
				ofs << i << "," <<  strbuff << "," << pupil_width[i] << "," << pupil_height[i] << endl;
				break;
			}
		}
	}
	ofs.close();
	return(0);
}

/** @function thresh_callback */
void thresh_callback(int, void* )
{
  Mat threshold_output;
  vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;

  /// Detect edges using Threshold
  threshold( src_gray, threshold_output, thresh, 255, THRESH_BINARY );
  // adaptiveThreshold( src_gray, threshold_output, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 5, thresh );
  
  
  /// Find contours
  findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

  /// Find the rotated rectangles and ellipses for each contour
//  vector<RotatedRect> minRect( contours.size() );
  //RotatedRect minEllipse;
//
//  for( int i = 0; i < contours.size(); i++ )
//     { minRect[i] = minAreaRect( Mat(contours[i]) );
//       if( contours[i].size() > 5 )
//         { minEllipse[i] = fitEllipse( Mat(contours[i]) ); }
//     }

  /// Draw contours + rotated rects + ellipses
  // Mat drawing = Mat::zeros( threshold_output.size(), CV_8UC3 );
 
  
  Mat drawing = src.clone();
  //Mat drawing = src_gray.clone();

  for( int i = 1; i< contours.size(); i++ ){
		//面積
		double Area = fabs(contourArea(contours[i]));
		//周囲長
		double Perimeter = arcLength(contours[i],true);
		//円形度
		double CircleLevel = 4.0 * CV_PI * Area / (Perimeter * Perimeter);

		double Flattening = 0;
		if (Area > downArea){
			//扁平率
			if (fitEllipse(Mat(contours[i])).size.height != 0){
				Flattening = fitEllipse(Mat(contours[i])).size.width / fitEllipse(Mat(contours[i])).size.height;
			}
		}

		printf("Circle level %d = %f, Area = %f, Flattening = %f\n", i, CircleLevel, Area, Flattening);

		if( CircleLevel > 0.5 && Area > downArea && Area < upArea && Flattening > 0.5){//2500
		   // Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
			Scalar color = Scalar( 0, 255, 0 );
			Scalar color2 = Scalar( 255, 0, 0 );

		   // contour
		   //drawContours( drawing, contours, i, color, 1, 8, vector<Vec4i>(), 0, Point() );

		   printf("size of contour = %d\n", contours[i].size());

		   minEllipse = fitEllipse( Mat(contours[i]) );
		   ellipse( drawing, minEllipse, color, 1, 8 );
		   if(get_iris){
			 //ellipse( drawing, minEllipse2, color2, 1, 8 );
		   }
		   printf("Pupil size = %f, %f\n", minEllipse.size.width, minEllipse.size.height);
		   pupil_width[i_x] = minEllipse.size.width;
		   pupil_height[i_x] = minEllipse.size.height;
		}
       // ellipse
 //      ellipse( drawing, minEllipse[i], color, 2, 8 );
       // rotated rectangle
 //      Point2f rect_points[4]; minRect[i].points( rect_points );
 //      for( int j = 0; j < 4; j++ )
  //        line( drawing, rect_points[j], rect_points[(j+1)%4], color, 1, 8 );
  }

  /// Show in a window
  namedWindow( "Source", CV_WINDOW_AUTOSIZE );
  imshow( "Source", drawing );
}

void thresh_callback2(int, void* )
{
  Mat threshold_output;
  vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;
  Scalar color = Scalar( 0, 255, 0 );
  Scalar color2 = Scalar( 255, 0, 0 );

  /// Detect edges using Threshold
  threshold( src_gray, threshold_output, iris_thresh, 255, THRESH_BINARY);
  //adaptiveThreshold( src_gray, threshold_output, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 5, iris_thresh );
  
  /// Find contours
  findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
 
  Mat drawing = src.clone();
  vector<Point> buff;

  for( int i = 1; i< contours.size(); i++ ){
		//面積
		double Area = fabs(contourArea(contours[i]));
		//周囲長
		double Perimeter = arcLength(contours[i],true);
		//円形度
		double CircleLevel = 4.0 * CV_PI * Area / (Perimeter * Perimeter);

		printf("Circle level %d = %f, Area = %f\n", i, CircleLevel, Area);
		printf("contours = %d\n",contours.size());
		
		if( CircleLevel > 0.5 && Area > 5000 ){//2500
			for(vector<Point>::iterator it = contours[i].begin();it != contours[i].end();){
				if((*it).x > 620 | (*it).x < 20 | (*it).y > 490 | (*it).y < 20){
					it = contours[i].erase(it);
				}
				else{
					++it;
				}
		   }
		   // contour
		   drawContours( drawing, contours, i, color, 1, 8, vector<Vec4i>(), 0, Point() );

		   printf("size of contour = %d\n", contours[i].size());

		   minEllipse2 = fitEllipse( Mat(contours[i]) );
		   ellipse( drawing, minEllipse2, color2, 1, 8 );
		   //printf("Pupil size = %f, %f\n", minEllipse2.size.width, minEllipse2.size.height);
		   //pupil_width2[i_x] = minEllipse2.size.width;
		   //pupil_height2[i_x] = minEllipse2.size.height;
		}
  }
  /// Show in a window
  namedWindow( "Source", CV_WINDOW_AUTOSIZE );
  imshow( "Source", drawing );
}

void Erosion(int, void*)
{
	int erosion_type;
	if (erosion_elem == 0){ erosion_type = MORPH_RECT; }
	else if (erosion_elem == 1){ erosion_type = MORPH_CROSS; }
	else if (erosion_elem == 2) { erosion_type = MORPH_ELLIPSE; }

	Mat element = getStructuringElement(erosion_type,
		Size(2 * erosion_size + 1, 2 * erosion_size + 1),
		Point(erosion_size, erosion_size));

	/// Apply the erosion operation
	erode(src_gray, src_gray, element);
	dilate(src_gray, src_gray, element);
}
