#include <jni.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <vector>
#include <android/log.h>
#include <math.h>

#define APPNAME "jni_part"

using namespace std;
using namespace cv;

//com.example.android.camera2basic.Camera2BasicFragment

extern "C" {
	/*JNIEXPORT jintArray JNICALL Java_ai_infrrd_infrrdocrframework_activities_DocumentProcessor_detectDocumentNative(
				JNIEnv*, jobject, jlong addrRgba,
				jdouble frameWidth, jdouble frameHeight); */


    /*JNIEXPORT jintArray JNICALL Java_ai_infrrd_infrrdocrframework_activities_DocumentPreviewActivity_detectDocumentNative(
    				JNIEnv*, jobject, jlong addrRgba,
    				jdouble frameWidth, jdouble frameHeight);*/

    //com.example.android.camera2basic.edgedetection.DocumentProcessor
    JNIEXPORT jintArray JNICALL
    Java_com_example_android_camera2basic_edgedetection_DocumentProcessor_detectDocumentNative(
    				JNIEnv*, jobject, jlong addrRgba,
    				jdouble frameWidth, jdouble frameHeight);

    JNIEXPORT void JNICALL Java_ai_infrrd_infrrdocrframework_activities_DocumentPreviewActivity_correctPerspectiveNative(
    				JNIEnv *env, jobject obj, jlong addrRgba, jintArray mTopLeft, jintArray mTopRight, jintArray mBottomLeft,
    				jintArray mBottomRight );


	jint rectTopLeftX;
	jint rectTopLeftY;
	jint mRectWidth;
	jint mRectHeight;
	jint topLeftX;
	jint topLeftY;
	jint topRightX;
	jint topRightY;
	jint bottomLeftX;
	jint bottomLeftY;
	jint bottomRightX;
	jint bottomRightY;


	double angle(Point pt1, Point pt2, Point pt0);
    bool checkBounds(vector<Point> approx, float, float);
    void assignCornersValues(vector<Point> approx);

	void findLargestSquare(const vector<vector<Point> >& squares, vector<Point>& biggest_square);
	void resetCornerValues();

	vector<vector<Point> > squares;

	vector<Point> approxGlobal;


	void findLargestSquare(const vector<vector<Point> >& squares, vector<Point>& biggest_square)
	{
	    if (!squares.size())
	    {
	        // no squares detected
	        return;
	    }

	    int max_width = 0;
	    int max_height = 0;
	    int max_square_idx = 0;
	    const int n_points = 4;

	    for (size_t i = 0; i < squares.size(); i++)
	    {
	        // Convert a set of 4 unordered Points into a meaningful cv::Rect structure.
	        Rect boundedRect = boundingRect(Mat(squares[i]));

	        // Store the index position of the biggest square found
	        if ((boundedRect.width >= max_width) && (boundedRect.height >= max_height))
	        {
	        	mRectWidth = max_width = boundedRect.width;
	        	mRectHeight = max_height = boundedRect.height;
	        	max_square_idx = i;

	        }
	    }

	    biggest_square = squares[max_square_idx];
	}


	JNIEXPORT jintArray JNICALL Java_ai_infrrd_infrrdocrframework_activities_DocumentProcessor_detectDocumentNative(
			JNIEnv *env, jobject obj, jlong addrRgba,
			jdouble frameWidth, jdouble frameHeight) {

			jlong imageCloned = addrRgba;
			Mat& image = *(Mat*)addrRgba;
			//image = enhanceImage(image);
			//cv::imwrite("/sdcard/Download/enhancedImage.jpg", image);
			resetCornerValues();
			Mat imageROI;

			if(!squares.empty())
				squares.clear();

			if(!squares.empty())
				approxGlobal.clear();

			imageROI = image;
			if(imageROI.empty()){
				return NULL;
			}
			int thresh = 50, N = 11;
			int found = 0;
			rectTopLeftX = 0;
			rectTopLeftY = 0;
			mRectWidth = 0;
			mRectHeight = 0;
			Mat pyr, timg, gray0(imageROI.size(), CV_8U), gray, grayCloned, image2;

			// down-scale and upscale the image to filter out the noise
			pyrDown(imageROI, pyr, Size(imageROI.cols/2, imageROI.rows/2));
			pyrUp(pyr, timg, imageROI.size());
			vector<vector<Point> > contours;

			vector<Point> finalCorners;
			// find squares in every color plane of the image
			for( int c = 0; c < 3; c++ )
			{

				int ch[] = {c, 0};
				mixChannels(&timg, 1, &gray0, 1, ch, 1);

				// try several threshold levels
				for( int l = 0; l < N; l++ )
				{
					// hack: use Canny instead of zero threshold level.
					// Canny helps to catch squares with gradient shading
					if( l == 0 )
					{
						// apply Canny. Take the upper threshold from slider
						// and set the lower to 0 (which forces edges merging)
						//__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "JNICall-JNI-Side-2");
						Canny(gray0, gray, 0, thresh, 3);
						// dilate canny output to remove potential
						// holes between edge segments
						dilate(gray, gray, Mat(), Point(-1,-1));
						grayCloned = gray.clone();
						//cv::HoughLinesP(gray, lines, 1, CV_PI/180, 50, 30, 40);
					}
					else
					{
						// apply threshold if l!=0:
						//     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
						gray = gray0 >= (l+1)*255/N;
					}

					// find contours and store them all as a list
					findContours(gray, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

					vector<Point> approx;

					// test each contour
					for( size_t i = 0; i < contours.size(); i++ )
					{
						// approximate contour with accuracy proportional
						// to the contour perimeter
						approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);

						if( approx.size() == 4 &&
							fabs(contourArea(Mat(approx))) > 1000 &&
							isContourConvex(Mat(approx)) )
						{

							double maxCosine = 0;

							for( int j = 2; j < 5; j++ )
							{
								// find the maximum cosine of the angle between joint edges
								double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
								maxCosine = MAX(maxCosine, cosine);
							}

							if( maxCosine < 0.3 ){
								Rect boundedRect = boundingRect(contours[i]);
								if (boundedRect.width > frameWidth*0.30 &&
										boundedRect.width < (frameWidth - 10)
										&& boundedRect.height > frameWidth*0.16
										&& boundedRect.height < (frameHeight - 10)) {

									rectTopLeftX = boundedRect.x;
									rectTopLeftY = boundedRect.y;

									squares.push_back(approx);
								}

								found = 1;
								jint result = (jint) found;

							}
						}
					}
				}
			}

			if(!squares.empty() && squares.size() > 0) {
				//__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "detected squares count %d", squares.size());
				findLargestSquare(squares, finalCorners);
				if(checkBounds(finalCorners, frameWidth, frameHeight))
					assignCornersValues(finalCorners);
				else
				    resetCornerValues();
			} else {
				resetCornerValues();
			}


			if (!image.empty()) {
				image.release();
				imageROI.release();
			}


			 jintArray result;
             result = env->NewIntArray(8);
             if (result == NULL) {
                 return NULL; /* out of memory error thrown */
             }
             int i;
             // fill a temp structure to use to populate the java int array
             jint cornerPoints[8];

             cornerPoints[0] = topLeftX;
             cornerPoints[1] = topLeftY;
             cornerPoints[2] = topRightX;
             cornerPoints[3] = topRightY;
             cornerPoints[4] = bottomLeftX;
             cornerPoints[5] = bottomLeftY;
             cornerPoints[6] = bottomRightX;
             cornerPoints[7] = bottomRightY;
             // move from the temp structure to the java structure
             env->SetIntArrayRegion(result, 0, 8, cornerPoints);
             return result;
	}


    JNIEXPORT jintArray JNICALL
    Java_com_example_android_camera2basic_edgedetection_DocumentProcessor_detectDocumentNative(
    			JNIEnv *env, jobject obj, jlong addrRgba,
    			jdouble frameWidth, jdouble frameHeight) {

    			jlong imageCloned = addrRgba;
    			Mat& image = *(Mat*)addrRgba;
    			//image = enhanceImage(image);
    			//cv::imwrite("/sdcard/Download/enhancedImage.jpg", image);
    			resetCornerValues();
    			Mat imageROI;

    			if(!squares.empty())
    				squares.clear();

    			if(!squares.empty())
    				approxGlobal.clear();

    			imageROI = image;
    			if(imageROI.empty()){
    				return NULL;
    			}
    			int thresh = 50, N = 11;
    			int found = 0;
    			rectTopLeftX = 0;
    			rectTopLeftY = 0;
    			mRectWidth = 0;
    			mRectHeight = 0;
    			Mat pyr, timg, gray0(imageROI.size(), CV_8U), gray, grayCloned, image2;

    			// down-scale and upscale the image to filter out the noise
    			pyrDown(imageROI, pyr, Size(imageROI.cols/2, imageROI.rows/2));
    			pyrUp(pyr, timg, imageROI.size());
    			vector<vector<Point> > contours;

    			vector<Point> finalCorners;
    			// find squares in every color plane of the image
    			for( int c = 0; c < 3; c++ )
    			{

    				int ch[] = {c, 0};
    				mixChannels(&timg, 1, &gray0, 1, ch, 1);

    				// try several threshold levels
    				for( int l = 0; l < N; l++ )
    				{
    					// hack: use Canny instead of zero threshold level.
    					// Canny helps to catch squares with gradient shading
    					if( l == 0 )
    					{
    						// apply Canny. Take the upper threshold from slider
    						// and set the lower to 0 (which forces edges merging)
    						//__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "JNICall-JNI-Side-2");
    						Canny(gray0, gray, 0, thresh, 3);
    						// dilate canny output to remove potential
    						// holes between edge segments
    						dilate(gray, gray, Mat(), Point(-1,-1));
    						grayCloned = gray.clone();
    						//cv::HoughLinesP(gray, lines, 1, CV_PI/180, 50, 30, 40);
    					}
    					else
    					{
    						// apply threshold if l!=0:
    						//     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
    						gray = gray0 >= (l+1)*255/N;
    					}

    					// find contours and store them all as a list
    					findContours(gray, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

    					vector<Point> approx;

    					// test each contour
    					for( size_t i = 0; i < contours.size(); i++ )
    					{
    						// approximate contour with accuracy proportional
    						// to the contour perimeter
    						approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);

    						if( approx.size() == 4 &&
    							fabs(contourArea(Mat(approx))) > 1000 &&
    							isContourConvex(Mat(approx)) )
    						{

    							double maxCosine = 0;

    							for( int j = 2; j < 5; j++ )
    							{
    								// find the maximum cosine of the angle between joint edges
    								double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
    								maxCosine = MAX(maxCosine, cosine);
    							}

    							if( maxCosine < 0.3 ){
    								Rect boundedRect = boundingRect(contours[i]);
    								if (boundedRect.width > frameWidth*0.30 &&
    										boundedRect.width < (frameWidth - 10)
    										&& boundedRect.height > frameWidth*0.16
    										&& boundedRect.height < (frameHeight - 10)) {

    									rectTopLeftX = boundedRect.x;
    									rectTopLeftY = boundedRect.y;

    									squares.push_back(approx);
    								}

    								found = 1;
    								jint result = (jint) found;

    							}
    						}
    					}
    				}
    			}

    			if(!squares.empty() && squares.size() > 0) {
    				//__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "detected squares count %d", squares.size());
    				findLargestSquare(squares, finalCorners);
    				if(checkBounds(finalCorners, frameWidth, frameHeight))
    					assignCornersValues(finalCorners);
    			} else {
    				resetCornerValues();
    			}


    			if (!image.empty()) {
    				image.release();
    				imageROI.release();
    			}


    			 jintArray result;
                 result = env->NewIntArray(8);
                 if (result == NULL) {
                     return NULL; /* out of memory error thrown */
                 }
                 int i;
                 // fill a temp structure to use to populate the java int array
                 jint cornerPoints[8];

                 cornerPoints[0] = topLeftX;
                 cornerPoints[1] = topLeftY;
                 cornerPoints[2] = topRightX;
                 cornerPoints[3] = topRightY;
                 cornerPoints[4] = bottomLeftX;
                 cornerPoints[5] = bottomLeftY;
                 cornerPoints[6] = bottomRightX;
                 cornerPoints[7] = bottomRightY;
                 // move from the temp structure to the java structure
                 env->SetIntArrayRegion(result, 0, 8, cornerPoints);
                 return result;
    }

	bool checkBounds(vector<Point> approx, float frameWidth, float frameHeight) {
		bool retValue;
		if(!approx.size())
			return false;
		int border = 1;
		if(approx[0].x > border && approx[0].x < frameWidth - border
				&& approx[0].y > border && approx[0].y < frameHeight - border
				&& approx[1].x > border && approx[1].x < frameWidth - border
				&& approx[1].y > border && approx[1].y < frameHeight - border
				&& approx[2].x > border && approx[2].x < frameWidth - border
				&& approx[2].y > border && approx[2].y < frameHeight - border
				&& approx[3].x > border && approx[3].x < frameWidth - border
				&& approx[3].y > border && approx[3].y < frameHeight - border)
			return true;
		else
			return false;
		return true;
	}

	void resetCornerValues() {
		topLeftX = 0;
		topLeftY = 0;

		topRightX = 0;
		topRightY = 0;

		bottomLeftX = 0;
		bottomLeftY = 0;
		bottomRightX = 0;
		bottomRightY = 0;
	}

	void assignCornersValues(vector<Point> approx) {
		topLeftX = approx[3].x;
		topLeftY = approx[3].y;

		topRightX = approx[0].x;
		topRightY = approx[0].y;

		bottomLeftX = approx[2].x;
		bottomLeftY = approx[2].y;
		bottomRightX = approx[1].x;
		bottomRightY = approx[1].y;

	}

	JNIEXPORT void JNICALL Java_ai_infrrd_infrrdocrframework_activities_DocumentPreviewActivity_correctPerspectiveNative(
				JNIEnv *env, jobject obj, jlong addrRgba, jintArray mTopLeft, jintArray mTopRight, jintArray mBottomLeft,
				jintArray mBottomRight ) {

        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "correctPerspectiveNative-1");

		Mat& srcImage = *(Mat*)addrRgba;
		std::vector<cv::Point2f> corners;

		jint *ptTopLeft = env->GetIntArrayElements(mTopLeft, NULL);
		jint *ptTopRight = env->GetIntArrayElements(mTopRight, NULL);
		jint *ptBottomLeft = env->GetIntArrayElements(mBottomLeft, NULL);
		jint *ptBottomRight = env->GetIntArrayElements(mBottomRight, NULL);

		float w1 = sqrt((ptBottomRight[0] - ptBottomLeft[0])*(ptBottomRight[0] - ptBottomLeft[0]) +
				(ptBottomRight[1] - ptBottomLeft[1])*(ptBottomRight[1] - ptBottomLeft[1]));
		float w2 = sqrt((ptTopRight[0] - ptTopLeft[0])*(ptTopRight[0] - ptTopLeft[0]) +
				(ptTopRight[1] - ptTopLeft[1])*(ptTopRight[1] - ptTopLeft[1]));


        float h1 = sqrt((ptBottomLeft[0] - ptTopLeft[0])*(ptBottomLeft[0] - ptTopLeft[0]) +
                				(ptBottomLeft[1] - ptTopLeft[1])*(ptBottomLeft[1] - ptTopLeft[1]));

	    float h2 = sqrt((ptBottomRight[0] - ptTopRight[0])*(ptBottomRight[0] - ptTopRight[0]) +
        				(ptBottomRight[1] - ptTopRight[1])*(ptBottomRight[1] - ptTopRight[1]));


		float maxWidth = (w1 > w2) ? w1 : w2;
		float maxHeight = (h1 > h2) ? h1 : h2;

		cv::Point topLeft;
		cv::Point topRight;
		cv::Point bottomRight;
		cv::Point bottomLeft;

		topLeft.x = ptTopLeft[0];
		topLeft.y = ptTopLeft[1];
		topRight.x = ptTopRight[0];
		topRight.y = ptTopRight[1];
		bottomRight.x = ptBottomRight[0];
		bottomRight.y = ptBottomRight[1];
		bottomLeft.x = ptBottomLeft[0];
		bottomLeft.y = ptBottomLeft[1];

		corners.push_back(topLeft);
		corners.push_back(topRight);
		corners.push_back(bottomLeft);
		corners.push_back(bottomRight);

		cv::Mat quad = cv::Mat::zeros(maxHeight, maxWidth, CV_8UC3);
		std::vector<cv::Point2f> quad_pts;
		quad_pts.push_back(cv::Point2f(0, 0));
		quad_pts.push_back(cv::Point2f(quad.cols, 0));
		quad_pts.push_back(cv::Point2f(0, quad.rows));
		quad_pts.push_back(cv::Point2f(quad.cols, quad.rows));

		cv::Mat transmtx = cv::getPerspectiveTransform(corners, quad_pts);
		cv::warpPerspective(srcImage, quad, transmtx, quad.size());

        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "correctPerspectiveNative-last-line");
		cv::imwrite("/sdcard/Download/InfrrdScanner/perspectiveCorrectedJNI.jpg", quad);

		//quad.release();
		//src.release();
		//imwrite("/sdcard/Download/perspectiveCorrectedJNI.jpg",quad);
		srcImage = quad;
		//return newImage;
	}

	JNIEXPORT jintArray JNICALL Java_com_authenticid_catfishair_activities_SquareDetectActivity_getTopLeft(
								JNIEnv *env, jobject obj) {

				 jintArray result;
				 result = env->NewIntArray(2);
				 if (result == NULL) {
				     return NULL; /* out of memory error thrown */
				 }
				 int i;
				 // fill a temp structure to use to populate the java int array
				 jint fill[2];

				 fill[0] = topLeftX;
				 fill[1] = topLeftY;
				 // move from the temp structure to the java structure
				 env->SetIntArrayRegion(result, 0, 2, fill);
				 return result;

		}

	JNIEXPORT jintArray JNICALL Java_com_authenticid_catfishair_activities_SquareDetectActivity_getTopRight(
								JNIEnv *env, jobject obj) {

				 jintArray result;
				 result = env->NewIntArray(2);
				 if (result == NULL) {
				     return NULL; /* out of memory error thrown */
				 }
				 int i;
				 // fill a temp structure to use to populate the java int array
				 jint fill[2];

				 fill[0] = topRightX;
				 fill[1] = topRightY;
				 // move from the temp structure to the java structure
				 env->SetIntArrayRegion(result, 0, 2, fill);
				 return result;

		}

	JNIEXPORT jintArray JNICALL Java_com_authenticid_catfishair_activities_SquareDetectActivity_getBottomLeft(
									JNIEnv *env, jobject obj) {

					 jintArray result;
					 result = env->NewIntArray(2);
					 if (result == NULL) {
					     return NULL; /* out of memory error thrown */
					 }
					 int i;
					 // fill a temp structure to use to populate the java int array
					 jint fill[2];

					 fill[0] = bottomLeftX;
					 fill[1] = bottomLeftY;
					 // move from the temp structure to the java structure
					 env->SetIntArrayRegion(result, 0, 2, fill);
					 return result;

			}

	JNIEXPORT jintArray JNICALL Java_com_authenticid_catfishair_activities_SquareDetectActivity_getBottomRight(
									JNIEnv *env, jobject obj) {

         jintArray result;
         result = env->NewIntArray(2);
         if (result == NULL) {
             return NULL; /* out of memory error thrown */
         }
         int i;
         // fill a temp structure to use to populate the java int array
         jint fill[2];

         fill[0] = bottomRightX;
         fill[1] = bottomRightY;
         // move from the temp structure to the java structure
         env->SetIntArrayRegion(result, 0, 2, fill);
         return result;

	}


	double angle(Point pt1, Point pt2, Point pt0) {
		double dx1 = pt1.x - pt0.x;
		double dy1 = pt1.y - pt0.y;
		double dx2 = pt2.x - pt0.x;
		double dy2 = pt2.y - pt0.y;
		return (dx1 * dx2 + dy1 * dy2)
				/ sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
	}

}
