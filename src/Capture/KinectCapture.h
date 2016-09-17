/* 
* Copyright (c) 2011 Steven Hickson Clemson University.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
* 
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#pragma once
#ifndef __BLEPO_KINECTCAPTURE_H__
#define __BLEPO_KINECTCAPTURE_H__

/////////////////////////////////////////////////////////
// This file contains routines to get information from the kinect
//
// This code uses the Microsoft Kinect SDK, which relies on the Microsoft drivers.
//    These drivers can be found at http://www.microsoft.com/en-us/kinectforwindows/
//
// An alternative would be to use OpenNI, which also does the basic capture, but does
//    not have person detection, skeleton detection, or point clouds.  PCL includes
//    OpenNI and is supposed to be able to produce point clouds, but we have not had
//    success using it.  -- 1/19/12

#include <assert.h>
#include <afx.h>  // CString
#include <windows.h>
#include <vector>
#include <algorithm>
//For Kinect Vision
#include "../../external/Microsoft/Kinect/NuiApi.h"
#include "../../external/Microsoft/Kinect/NuiImageCamera.h"
#include "../../external/Microsoft/Kinect/NuiSkeleton.h"
#include "../../external/Microsoft/Kinect/NuiSensor.h"

//For Kinect Audio
// For configuring DMO properties
#include <wmcodecdsp.h>
// For discovering microphone array device
//#include <MMDeviceApi.h>
//#include <devicetopology.h>
#include "../Utilities/Exception.h"  // BLEPO_ERROR, Exception
#include "../Image/Image.h"
#include "../Image/ImageOperations.h"
#include "../Matrix/Matrix.h"
#include "../Matrix/MatrixOperations.h"
#include "../Matrix/LinearAlgebra.h"
#include "../Figure/FigureGlut.h"

using namespace std;

namespace blepo
{
	//DEFINES constants for kinect cameras
	#define KINECT_CX_C 338.94272028759258
	#define KINECT_CY_C 232.51931828128443
	#define KINECT_FX_C 529.21508098293293
	#define KINECT_FY_C 525.56393630057437

	
	//I preinvert all the depth focal points so they can be multiplied instead of divided. 
	//3.3930780975300314e+02
	//#define KINECT_CX_D 3.3930780975300314e+02
	//#define KINECT_CY_D 2.4273913761751615e+02
	#define KINECT_CX_D 3.3330780975300314e+02
	#define KINECT_CY_D 2.2273913761751615e+02
	#define KINECT_FX_D 1.6828944189289601e-03
	#define KINECT_FY_D 1.6919313269589566e-03
	/*
	#define KINECT_CX_C 3.2894272028759258e+02
	//2.6748068171871557e+02
	#define KINECT_CY_C 2.1151931828128443e+02
	#define KINECT_FX_C 5.2921508098293293e+02
	#define KINECT_FY_C 5.2556393630057437e+02
	*/
	//rotation and translation
	#define KINECT_R11 9.9984628826577793e-01
	#define KINECT_R12 1.2635359098409581e-03
	#define KINECT_R13 -1.7487233004436643e-02
	#define KINECT_R21 -1.4779096108364480e-03
	#define KINECT_R22 9.9992385683542895e-01
	#define KINECT_R23 -1.2251380107679535e-02
	#define KINECT_R31 1.7470421412464927e-02
	#define KINECT_R32 1.2275341476520762e-02
	#define KINECT_R33 9.9977202419716948e-01
	#define KINECT_T1 1.9985242312092553e-02
	#define KINECT_T2 -7.4423738761617583e-04
	#define KINECT_T3 -1.0916736334336222e-02
	
	//I'm putting these functions outside of the kinect class so you don't have to have a kinect plugged in
	void GetPointCloudFromKinectData(const ImgBgr &img, const ImgInt &depth, PointCloud *cloud, bool useZeros);
	inline Point KinectDepthToColorCoord(const ImgBgr &img, float x, float y, float z);
	void ColorCoordtoKinectDepth(const ImgFloat &img, CPoint &pt, float *x, float *y, float *z);

	class Point3D {
	public:
		float x,y,z;

		Point3D() {
			x = y = z = 0.0f;
		};

		Point3D(float inX, float inY, float inZ) {
			x = inX;
			y = inY;
			z = inZ;
		};
	};

	class KinectCapture 
	{
	private:
		HANDLE videoStreamHandle;
		HANDLE depthStreamHandle;
		bool depthStarted, videoStarted;
		INuiSensor *kinectInstance;
		int depthWidth, depthHeight, imgWidth, imgHeight;
		bool fullDepth, shiftedDepth;
		Rect m_roi;
		bool IAmRoied;
		ImgBgr origImg, depthwPerson;
		ImgInt origDepth;
		bool useMicrosoftMapping;

	public:
		// instance:  zero-based index of Kinect sensor (set to zero if only one sensor)
		// image:  whether to capture RGB images (640 x 480)
		// depth:  whether to capture depth images (640 x 480 unless player=true or skeleton==true, in which case it's 320 x 240)
		// player:  whether to capture person images (i.e., segmented/detected people in depth images) (320 x 240)
		// skeleton:  whether to capture skeletons of people detected (320 x 240)
		KinectCapture(int instance = 0, bool image = true, bool depth = true, bool player = false, bool skeleton = false);
		~KinectCapture();

		void SetRoi(const Rect &roi)
		{
			m_roi = roi;
			IAmRoied = true;
		};

		Rect GetRoi() const 
		{
			return m_roi;
		};

		void ClearRoi() 
		{
			m_roi = Rect(0,0,640,480);
			IAmRoied = false;
		};

		void ClearRoiAndGetOldData(ImgBgr *img, ImgInt *depth) 
		{
			m_roi = Rect(0,0,640,480);
			IAmRoied = false;
			*img = origImg;
			*depth = origDepth;
		};

		void StartVideoCapture();
		void StartDepthCapture();
		void StartAudioCapture();

		// Note:  These Get functions actually talk to the Kinect to get the data; may require a brief (milliseconds) wait

		bool GetFrame(blepo::ImgBgr *img);

		// shifted == false means return the raw depth values (lowest 3 bits encode person information)
		//            (units would be millimeters if shifted to the right by 3 bits)
		// shifted == true means depth values have been shifted right by 3 bits to remove person information
		//            (units are millimeters)
		bool GetDepth(blepo::ImgInt *img, bool shifted = true);

		bool GetDepthAndPerson(blepo::ImgBgr *img);

		bool GetSkeleton(NUI_SKELETON_FRAME *skeleton);

		// warning:  do not move too often (Taken care of)
		void TiltKinect(int move);

		//x and y should be the depth indices, z should be the depth in meters (what the point cloud gives you)
		Point GetImageDataFromDepthData(int x, int y, float z);
		void GetPointCloudFromData(const ImgBgr &img, const ImgInt &depth, PointCloud *cloud, bool useZeros = false);

		void GetPointCloud(PointCloud *cloud, bool useZeros = false);
		
		void GetData(blepo::ImgBgr *img, blepo::ImgInt *depth, PointCloud *cloud, bool useZeros = false);
		
		//void GetData(blepo::ImgBgr *img, blepo::ImgInt *depth, PointCloud *cloud); //for skeleton and other stuff
		//void GetData(blepo::ImgBgr *img, blepo::ImgInt *depth, PointCloud *cloud);
	
		//bool BeamLocalization(IMediaObject* pDMO, IPropertyStore* pPS, int msDuration);
	};

};

#endif // __BLEPO_KINECTCAPTURE_H__
