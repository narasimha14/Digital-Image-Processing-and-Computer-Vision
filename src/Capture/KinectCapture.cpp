/* 
* Copyright (c) 2011 Steven HicksonClemson University.
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
#ifndef __BLEPO_KINECTCAPTURE_C__
#define __BLEPO_KINECTCAPTURE_C__

/////////////////////////////////////////////////////////
// This file contains routines to get information from the kinect
#include "KinectCapture.h"

#pragma region ExtraFunctions

inline USHORT ShiftDepth(USHORT depth) {
	return (depth & 0xfff8) >> 3;
}

inline void ShortToBgr_Depth( USHORT s, blepo::Bgr *pixel)
{
	USHORT RealDepth = ShiftDepth(s);
	USHORT Player = s & 7;

	// transform 13-bit depth information into an 8-bit intensity appropriate
	// for display (we disregard information in most significant bit)
	BYTE l = 255 - (BYTE)(256*RealDepth/0x0fff);

	pixel->b = pixel->g = pixel->r = 0;

	switch( Player )
	{
	case 0:
		pixel->r = l / 2;
		pixel->b = l / 2;
		pixel->g = l / 2;
		break;
	case 1:
		pixel->r = l;
		break;
	case 2:
		pixel->g = l;
		break;
	case 3:
		pixel->r = l / 4;
		pixel->g = l;
		pixel->b = l;
		break;
	case 4:
		pixel->r = l;
		pixel->g = l;
		pixel->b = l / 4;
		break;
	case 5:
		pixel->r = l;
		pixel->g = l / 4;
		pixel->b = l;
		break;
	case 6:
		pixel->r = l / 2;
		pixel->g = l / 2;
		pixel->b = l;
		break;
	case 7:
		pixel->r = 255 - ( l / 2 );
		pixel->g = 255 - ( l / 2 );
		pixel->b = 255 - ( l / 2 );
	}
}

inline float RawDepthToMeters(int depthValue)
{
	if (depthValue >= 800 && depthValue <= 4000) {
		return float(depthValue) * 0.001f;
	}
	return 0.0f;
}


inline blepo::Point3D DepthToWorld(int x, int y, int depthValue, bool fullDepth)
{    
	blepo::Point3D result;
	double depth = RawDepthToMeters(depthValue);
	result.x = float((x - KINECT_CX_D) * depth * KINECT_FX_D);
	result.y = float((y - KINECT_CY_D) * depth * KINECT_FY_D);
	result.z = float(depth);
	return result;
}

namespace blepo
{

inline Point KinectDepthToColorCoord(const ImgBgr &img, float x, float y, float z) {
	Point ret;
	float x_p, y_p, z_p, cx_c = (float) KINECT_CX_C, cy_c = (float) KINECT_CY_C;
	if(img.Width() == 320) {
		cx_c /= 2;
		cy_c /= 2;
	}
	x_p = float(KINECT_R11 * x + KINECT_R12 * y + KINECT_R13 * z);
	y_p = float(KINECT_R21 * x + KINECT_R22 * y + KINECT_R23 * z);
	z_p = float(1.0f / (KINECT_R31 * x + KINECT_R32 * y + KINECT_R33 * z));
	int safeHeight = img.Height() - 1, safeWidth = img.Width() - 1;
	//ret.x = blepo_ex::Clamp<int>(safeWidth - int(x_p * KINECT_FX_C * z_p + cx_c),0,safeWidth);
	ret.x = blepo_ex::Clamp<int>(int(x_p * KINECT_FX_C * z_p + cx_c),0,safeWidth);
	ret.y = blepo_ex::Clamp<int>(safeHeight - int(y_p * KINECT_FX_C * z_p + cy_c),0,safeHeight);
	return ret;
}

void ColorCoordtoKinectDepth(const ImgFloat &img, CPoint &pt, float *x, float *y, float *z) {
	float cx_c = (float) KINECT_CX_C, cy_c = (float) KINECT_CY_C;
	int safeHeight = img.Height() - 1, safeWidth = img.Width() - 1;
	MatDbl rotate(3,3),wc(1,3),cc(1,3),invrotate(3,3);

	if(img.Width() == 320) {
		cx_c /= 2;
		cy_c /= 2;
	}
	cc(0,0)=(pt.x-cx_c)/KINECT_FX_C;
	cc(0,1)=(cy_c+safeHeight-pt.y)/KINECT_FX_C;
	cc(0,2)=img(pt.x,pt.y);

	rotate(0,0)=KINECT_R11;
	rotate(1,0)=KINECT_R12;
	rotate(2,0)=KINECT_R13;
	rotate(0,1)=KINECT_R21;
	rotate(1,1)=KINECT_R22;
	rotate(2,1)=KINECT_R23;
	rotate(0,2)=KINECT_R31;
	rotate(1,2)=KINECT_R32;
	rotate(2,2)=KINECT_R33;

	Inverse3x3(rotate,&invrotate);
	wc=invrotate*cc;

	(*x)=(float) wc(0,0);
	(*y)=(float) wc(0,1);
	(*z)=(float) wc(0,2);
}

void GetPointCloudFromKinectData(const ImgBgr &img, const ImgInt &depth, PointCloud *cloud, bool useZeros)
{
	//take care of old cloud to prevent memory leak/corruption
	if (cloud != NULL && cloud->Size() > 0) {
		//cloud->ReleaseData();
	}
	cloud->Resize(img.Width()*img.Height());
	cloud->Set(ColoredPoint(0.0f,0.0f,0.0f,Bgr::BLACK,false));
	ImgInt::ConstIterator pDepth = depth.Begin();
	int safeWidth = img.Width() - 1, safeHeight = img.Height() - 1;
	bool fullDepth = true;
	float cx_d = (float) KINECT_CX_D, cy_d = (float) KINECT_CY_D;
	if(depth.Width() == 320) {
		fullDepth = false;
		cx_d /= 2;
		cy_d /= 2;
	}
	for(int j = 0; j < depth.Height(); j++) {
		for(int i = 0; i < depth.Width(); i++) {
			Point3D loc;
			//LONG x = fullDepth ? i >> 1 : i, y = fullDepth ? j >> 1 : j;
			if(useZeros && *pDepth == 0) {
				loc.x = float(((float)i - cx_d) * KINECT_FX_D);
				loc.y = float(((float)(safeHeight - j) - cy_d) * KINECT_FY_D);
				loc.z = 0;
			} else {
				const double newDepth = (*pDepth * 0.001f); //convert from millimeters to meters
				loc.x = float(((float)i - cx_d) * newDepth * KINECT_FX_D);
				//changed to fix bug, old code: loc.y = float((safeHeight - (y - KINECT_CY_D)) * newDepth * KINECT_FY_D); //I'm pretty dumb
				loc.y = float(((float)(safeHeight - j) - cy_d) * newDepth * KINECT_FY_D);
				loc.z = float(newDepth);
			}
			//printf("i: %d j: %d x: %f y: %f z: %f Color_X: %d Color_Y: %d\n",i,j,loc.x,loc.y,loc.z,x,y);
			Point colorLoc = KinectDepthToColorCoord(img,loc.x,loc.y,loc.z);
			//x = blepo_ex::Clamp<int>(safeWidth-x,0,safeWidth);
			//y = blepo_ex::Clamp<int>(y,0,safeHeight);
			int x = colorLoc.x;
			int y = colorLoc.y;
			Bgr color = img(x,y);
			cloud->Set(ColoredPoint(loc.x,loc.y,loc.z,color,*pDepth!=0),x + y * img.Width());
			/*if (!fullDepth && y < img.Height()) {
				cloud->Set(ColoredPoint(loc.x,loc.y,loc.z,img(x,y+1),*pDepth!=0),x+img.Width()*(y+1));
				//cloud->Add(ColoredPoint(loc.x,loc.y,loc.z,img(colorLoc.x,colorLoc.y+1),*pDepth!=0));
			}*/
			pDepth++;
		}
	}
}

#pragma endregion

	//Connecting Functions
#pragma region Capture

  KinectCapture::KinectCapture(int instance, bool image, bool depth, bool player, bool skeleton)
  {
		//HRESULT hr;
		int num = 1;
		videoStreamHandle = NULL;
		depthStreamHandle = NULL;
		HRESULT hr = NuiGetSensorCount(&num);
		if ( FAILED( hr ) )
			BLEPO_ERROR("Failed to find Kinect sensor");

		int options = 0;
		if (image) {
			options |= NUI_INITIALIZE_FLAG_USES_COLOR;
			imgWidth = 640;
			imgHeight = 480;
		}
		if ((depth && player) | skeleton) {
			options |= NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX;
			depthWidth = 320;
			depthHeight = 240;
			fullDepth = false;
		} else if (depth) {
			options |= NUI_INITIALIZE_FLAG_USES_DEPTH;
			depthWidth = 640;
			depthHeight = 480;
			fullDepth = true;
		}
		m_roi = Rect(0,0,depthWidth,depthHeight);
		IAmRoied = false;
		if (skeleton)
			options |= NUI_INITIALIZE_FLAG_USES_SKELETON;
		if (num > instance) {
			hr = NuiCreateSensorByIndex(num - 1, &kinectInstance);
			if ( FAILED( hr ) ) 
			{
				BLEPO_ERROR("Failed to connect to Kinect sensor");
			}
			hr = kinectInstance->NuiStatus();
			if ( FAILED( hr ) ) {
				BLEPO_ERROR("Kinect Sensor has improper status");
			}
			depthStarted = videoStarted = false;
			hr = kinectInstance->NuiInitialize(options);
			if ( FAILED( hr ) )
				BLEPO_ERROR("Failed to initialize Kinect sensor");
		} else
			BLEPO_ERROR("Failed to Find a Kinect sensor");
	}

  KinectCapture::~KinectCapture()
  {
		kinectInstance->NuiShutdown();
		if (videoStreamHandle != NULL) {
			//CloseHandle(videoStreamHandle);
			videoStreamHandle = NULL;
		}
		if (depthStreamHandle != NULL) {
			//CloseHandle(depthStreamHandle);
			depthStreamHandle = NULL;
		}
	}

	void KinectCapture::StartVideoCapture() {
		HRESULT hr = kinectInstance->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR,NUI_IMAGE_RESOLUTION_640x480,0,2,0,&videoStreamHandle);
		if (FAILED( hr )) {
			BLEPO_ERROR("Failed to Open Kinect Image Stream");
		} else
			videoStarted = true;
	}

	void KinectCapture::StartDepthCapture() {
		HRESULT hr;
		if (fullDepth)
			hr = kinectInstance->NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH,NUI_IMAGE_RESOLUTION_640x480,0,2,0,&depthStreamHandle);
		else
			hr = kinectInstance->NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX,NUI_IMAGE_RESOLUTION_320x240,0,2,0,&depthStreamHandle);
		if ( FAILED( hr ) ) {
			BLEPO_ERROR("Failed to Open Kinect Depth Stream");
		} else
			depthStarted = true;
	}

#pragma endregion

	//Camera Functions
#pragma region Camera

	bool KinectCapture::GetFrame(blepo::ImgBgr *img) {
		if (!videoStarted)
			StartVideoCapture();

		NUI_IMAGE_FRAME pImageFrame;
		HRESULT hr = kinectInstance->NuiImageStreamGetNextFrame(videoStreamHandle,0,&pImageFrame);
		if ( FAILED( hr ) )
			return false;
		bool ret = false;
		INuiFrameTexture * pTexture = pImageFrame.pFrameTexture;
		NUI_LOCKED_RECT LockedRect;
		pTexture->LockRect( 0, &LockedRect, NULL, 0 );
		if ( LockedRect.Pitch != 0 ) //should be in this if statement otherwise erronous info
		{
			origImg.Reset(imgWidth,imgHeight);
			//Get Image info and put it into new image (weird arithmetic is to flip the image on the fly)
			BYTE * pBuffer = (BYTE*) LockedRect.pBits;
			int safeWidth = imgWidth - 1, count = safeWidth, loop = 0, multiplier = imgWidth << 1;
			blepo::ImgBgr::Iterator pOut = origImg.Begin(count,0);
			while(loop < imgHeight) {
				*pOut-- = Bgr(*pBuffer,*(pBuffer+1),*(pBuffer+2));
				count--;
				pBuffer += 4;
				if (count < 0) {
					loop++;
					pOut += multiplier;
					count = safeWidth;
				}
			}
			if (IAmRoied)
				Extract(origImg,m_roi,img);
			else
				*img = origImg;
			ret = true;
		}
		// We're done with the texture so unlock it
		pTexture->UnlockRect(0);
		kinectInstance->NuiImageStreamReleaseFrame( videoStreamHandle, &pImageFrame );
		return ret;
	}

#pragma endregion

	//Depth Functions
#pragma region Depth

	bool KinectCapture::GetDepth(blepo::ImgInt *img, bool shifted)
  {
		if (!depthStarted)
			StartDepthCapture();
		NUI_IMAGE_FRAME pImageFrame;
		shiftedDepth = shifted;
		HRESULT hr = kinectInstance->NuiImageStreamGetNextFrame(depthStreamHandle,0,&pImageFrame );

		if ( FAILED( hr ) )
			return false;
		bool ret = false;
		INuiFrameTexture * pTexture = pImageFrame.pFrameTexture;
		NUI_LOCKED_RECT LockedRect;
		pTexture->LockRect( 0, &LockedRect, NULL, 0 );
		if ( LockedRect.Pitch != 0 )
		{
			origDepth.Reset(depthWidth,depthHeight);
			BYTE * pBuffer = (BYTE*) LockedRect.pBits;

			// draw the bits to the bitmap
			blepo::ImgInt::Iterator pOut = origDepth.Begin();
			USHORT * pBufferRun = (USHORT*) pBuffer;
			for( int y = 0 ; y < depthHeight ; y++ )
			{
				for( int x = 0 ; x < depthWidth ; x++ )
				{
					USHORT RealDepth;
					if (shifted && !fullDepth)
						RealDepth = (*pBufferRun++ & 0xfff8) >> 3;
					else
						RealDepth = *pBufferRun++;
					*pOut++ = (int)RealDepth;
				}
			}
			if (IAmRoied)
      {
				if (fullDepth)
					Extract(origDepth,m_roi,img);
				else
					Extract(origDepth,Rect(m_roi.left >> 1,m_roi.top >> 1,m_roi.right >> 1,m_roi.bottom >> 1),img);
			} else
				*img = origDepth;
			ret = true;
		} 
		pTexture->UnlockRect(0);
		kinectInstance->NuiImageStreamReleaseFrame(depthStreamHandle, &pImageFrame );
	
    //Have to flip for some reason; would be more efficient to do this as it comes off the sensor
		FlipHorizontal(*img,img);
		return ret;
	}

	bool KinectCapture::GetDepthAndPerson(blepo::ImgBgr *img) {
		if (!depthStarted)
			StartDepthCapture();

		NUI_IMAGE_FRAME pImageFrame;

		HRESULT hr = kinectInstance->NuiImageStreamGetNextFrame(depthStreamHandle,0,&pImageFrame );

		if ( FAILED( hr ) )
			return false;
		bool ret = false;
		INuiFrameTexture * pTexture = pImageFrame.pFrameTexture;
		NUI_LOCKED_RECT LockedRect;
		pTexture->LockRect( 0, &LockedRect, NULL, 0 );
		if ( LockedRect.Pitch != 0 )
		{
			img->Reset(depthWidth,depthHeight);
			BYTE * pBuffer = (BYTE*) LockedRect.pBits;

			// draw the bits to the bitmap
			blepo::ImgBgr::Iterator pOut = img->Begin();
			USHORT * pBufferRun = (USHORT*) pBuffer;
			for( int y = 0 ; y < depthHeight ; y++ )
			{
				for( int x = 0 ; x < depthWidth ; x++ )
				{
					blepo::Bgr pixel;
					ShortToBgr_Depth( *pBufferRun++, &pixel );
					*pOut++ = pixel;
				}
			}
			ret = true;
		} 
		pTexture->UnlockRect(0);
		kinectInstance->NuiImageStreamReleaseFrame(depthStreamHandle, &pImageFrame );
		return ret;
	}

#pragma endregion

	//Skeleton functions
#pragma region Skeleton

	bool KinectCapture::GetSkeleton(NUI_SKELETON_FRAME *skeleton) {

		HRESULT hr = kinectInstance->NuiSkeletonGetNextFrame( 0, skeleton );

		bool foundSkeleton = false;
		for( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
		{
			if ( skeleton->SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED )
			{
				foundSkeleton = true;
			}
		}

		// no skeletons!
		//
		if ( !foundSkeleton )
		{
			return false;
		}

		// smooth out the skeleton data
		kinectInstance->NuiTransformSmooth(skeleton,NULL);

		// draw each skeleton color according to the slot within they are found.
		//
		return true;

	}

	void KinectCapture::TiltKinect(int move) 
  {
		assert(move >= -28 && move <= 28);
		kinectInstance->NuiCameraElevationSetAngle(move);
	}

#pragma endregion

	//Point Cloud Functions
#pragma region Point Cloud

	Point KinectCapture::GetImageDataFromDepthData(int x, int y, float z) {
		int safeWidth = imgWidth - 1, safeHeight = imgHeight - 1;
		//x = x >> 1;
		//y = y >> 1;
		LONG newX = LONG(x), newY = LONG(y);
		kinectInstance->NuiImageGetColorPixelCoordinatesFromDepthPixel(NUI_IMAGE_RESOLUTION_640x480,NULL,LONG(x),LONG(y),fullDepth ? USHORT(z)<<3 : USHORT(z),&newX,&newY);
		//newX = newX << 1;
		//newY = newY << 1;
		newX = blepo_ex::Clamp<int>(safeWidth-newX,0,safeWidth);
		newY = blepo_ex::Clamp<int>(newY,0,safeHeight);
		return Point(int(newX),int(newY));
	}

	void KinectCapture::GetPointCloudFromData(const ImgBgr &img, const ImgInt &depth, PointCloud *cloud, bool useZeros)
  {
		//take care of old cloud to prevent memory leak/corruption
		if (cloud != NULL && cloud->Size() > 0) {
			cloud->ReleaseData();
		}
		cloud->Resize(img.Width()*img.Height());
		cloud->Set(ColoredPoint(0.0f,0.0f,0.0f,Bgr::BLACK,false));
		ImgInt::ConstIterator pDepth = depth.Begin();
		int safeWidth = img.Width() - 1, safeHeight = img.Height() - 1;
		float cx_d = (float) KINECT_CX_D, cy_d = (float) KINECT_CY_D;
		if(!fullDepth) {
			cx_d /= 2;
			cy_d /= 2;
		}
		for(int j = m_roi.top; j < m_roi.bottom; j++) {
			for(int i = m_roi.left; i < m_roi.right; i++) {
					Point3D loc;
					LONG x = fullDepth ? i >> 1 : i, y = fullDepth ? j >> 1 : j;
					kinectInstance->NuiImageGetColorPixelCoordinatesFromDepthPixel(NUI_IMAGE_RESOLUTION_640x480,NULL,LONG(320-x),LONG(y),(fullDepth ? (*pDepth)<<3 : *pDepth),&x,&y);
					if(!fullDepth) {
						x /= 2;
						y /= 2;
					}
					x = blepo_ex::Clamp<int>(safeWidth-x,0,safeWidth);
					y = blepo_ex::Clamp<int>(y,0,safeHeight);
					if(useZeros && *pDepth == 0) {
						loc.x = float(((float)i - cx_d) * KINECT_FX_D);
						loc.y = float(((float)(safeHeight - j) - cy_d) * KINECT_FY_D);
						loc.z = 0;
					} else {
						const double newDepth = (*pDepth * 0.001f); //convert from millimeters to meters
						loc.x = float(((float)i - cx_d) * newDepth * KINECT_FX_D);
						//changed to fix bug, old code: loc.y = float((safeHeight - (y - KINECT_CY_D)) * newDepth * KINECT_FY_D); //I'm pretty dumb
						loc.y = float(((float)(safeHeight - j) - cy_d) * newDepth * KINECT_FY_D);
						loc.z = float(newDepth);
					}
					//printf("i: %d j: %d x: %f y: %f z: %f Color_X: %d Color_Y: %d\n",i,j,loc.x,loc.y,loc.z,x,y);
					/*Point colorLoc = DepthToColorCoord(loc.x,loc.y,loc.z);
					int x = colorLoc.x;
					int y = colorLoc.y;*/
					Bgr color = img(x,y);
					cloud->Set(ColoredPoint(loc.x,loc.y,loc.z,color,*pDepth!=0),x + y * img.Width());
					/*if (!fullDepth && y < imgHeight) {
						cloud->Set(ColoredPoint(loc.x,loc.y,loc.z,img(x,y+1),*pDepth!=0),x+imgWidth*(y+1));
						//cloud->Add(ColoredPoint(loc.x,loc.y,loc.z,img(colorLoc.x,colorLoc.y+1),*pDepth!=0));
					}*/
				pDepth++;
			}
		}
	}

	void KinectCapture::GetPointCloud(PointCloud *cloud, bool useZeros) {
		ImgInt depth;
		ImgBgr img;
		GetData(&img,&depth,cloud, useZeros);
	}

	void KinectCapture::GetData(blepo::ImgBgr *img, blepo::ImgInt *depth, PointCloud *cloud, bool useZeros) {
		int attempts = 0; //give ten tries, sometimes is necessary on startup or if code is running really fast?
		//this number is temporary, should put a timer with 100 ms or so here.
		int totAttempts = 10000;
		while(!GetDepth(depth) && attempts < totAttempts) {
			Sleep(1);
			attempts++;
		}
		if (attempts == totAttempts)
			BLEPO_ERROR("Couldn't Get Depth");
		attempts = 0;
		while(!GetFrame(img) && attempts < totAttempts) {
			Sleep(1);
			attempts++;
		}
		if (attempts == totAttempts)
			BLEPO_ERROR("Couldn't Get Image");
		GetPointCloudFromData(*img,*depth,cloud,useZeros);
	
	}


#pragma endregion

	//Audio Functions
#pragma region Audio

	void KinectCapture::StartAudioCapture() {
		/*HRESULT hr = S_OK;
		CoInitialize(NULL);
		IMediaObject* pDMO = NULL;  
		IPropertyStore* pPS = NULL;

		// control how long the Demo runs
		int  iDuration = 20;   // seconds

		// Set high priority to avoid getting preempted while capturing sound
		//mmHandle = AvSetMmThreadCharacteristics(L"Audio", &mmTaskIndex);
		//CHECK_BOOL(mmHandle != NULL, "failed to set thread priority\n");


		// DMO initialization
		hr = CoCreateInstance(CLSID_CMSRKinectAudio, NULL, CLSCTX_INPROC_SERVER, IID_IMediaObject, (void**)&pDMO);
		if (FAILED(hr)) {
		BLEPO_ERROR("Could not cocreateinstance");
		return;
		}
		hr = pDMO->QueryInterface(IID_IPropertyStore, (void**)&pPS);
		if (FAILED(hr)) {
		BLEPO_ERROR("Could not query interface");
		return;
		}
		// Set AEC-MicArray DMO system mode.
		// This must be set for the DMO to work properly
		PROPVARIANT pvSysMode;
		PropVariantInit(&pvSysMode);
		pvSysMode.vt = VT_I4;
		//   SINGLE_CHANNEL_AEC = 0
		//   OPTIBEAM_ARRAY_ONLY = 2
		//   OPTIBEAM_ARRAY_AND_AEC = 4
		//   SINGLE_CHANNEL_NSAGC = 5
		pvSysMode.lVal = (LONG)(4);
		hr = pPS->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode);
		if (FAILED(hr)) {
		BLEPO_ERROR("Could not Set System mode in DMO");
		return;
		}
		PropVariantClear(&pvSysMode);

		// Tell DMO which capture device to use (we're using whichever device is a microphone array).
		// Default rendering device (speaker) will be used.
		//TEMP hr = GetMicArrayDeviceIndex(&iMicDevIdx);
		if (FAILED(hr)) {
		BLEPO_ERROR("Failed to find microphone array device. Make sure microphone array is properly installed.");
		}*/
	}
	/*
	bool KinectCapture::BeamLocalization(IMediaObject* pDMO, IPropertyStore* pPS, int msDuration) {
	ISoundSourceLocalizer* pSC = NULL;
	HRESULT hr;
	int  cTtlToGo = 0;
	hr = pDMO->QueryInterface(IID_ISoundSourceLocalizer, (void**)&pSC);
	if (FAILED(hr)) {
	BLEPO_ERROR("QueryInterface for IID_ISoundSourceLocalizer failed");
	}
	float endTime = float(msDuration) / 1000.0f;
	PerformanceTimer time;
	time.Start();
	while(time.Duration() < endTime) {
	double dBeamAngle, dAngle;
	hr = pSC->GetBeam(&dBeamAngle);
	double dConf = 0;
	hr = pSC->GetPosition(&dAngle, &dConf);
	if (SUCCEEDED(hr) && dConf > 0.9)											
	printf_s("Position: %f\t\tConfidence: %f\t\tBeam Angle = %f\r", dAngle, dConf, dBeamAngle);					
	}
	time.Stop();
	if (pSC != NULL) {
	pSC->Release(); 
	pSC = NULL;
	}
	return true;
	}*/

#pragma endregion

};

#endif // __BLEPO_KINECTCAPTURE_C__
