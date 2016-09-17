#pragma once
#ifndef _BLEPO_SURF_H__
#define __BLEPO_SURF_H__

#include "Image\Image.h"
#include "Image\ImageOperations.h"
#include <vector>
#include <assert.h>
#include "Image\ImgIplImage.h"


using namespace std;
namespace blepo {

class SURFDescriptor{
public:
	bool			valid;
	float 			x, y, radius;
	//128 or 64 floats depending on whether extended is chosen or not
	float 			*descriptor;	
	SURFDescriptor		*match;
};

class SURF{
public:
	typedef SURFDescriptor* Iterator;
	typedef SURFDescriptor* ConstIterator;

	SURF(double hessianThreshold = 500, bool extended = true) //good values are (500, true)
		: m_threshold(hessianThreshold), m_extended(extended), m_CvSeqKeypoints(NULL), m_CvSeqDescriptors(NULL), m_descriptors(NULL), m_size(0) {}

	~SURF() {
	}

	//must release before done using, not included in the destructor for shallow copying purposes
	void Release() {
		if(m_storage != NULL) {
			cvClearMemStorage(m_storage);
			cvReleaseMemStorage(&m_storage);
			m_storage = NULL;
		}
		if(m_descriptors != NULL) {
			free(m_descriptors);
			m_descriptors = NULL;
		};
		m_size = 0;
	}

	inline Iterator Begin() {
		return m_descriptors;
	}

	inline Iterator End() {
		return m_descriptors + m_size;
	}

	inline ConstIterator Begin() const {
		return m_descriptors;
	}

	inline ConstIterator End() const {
		return m_descriptors + m_size;
	}

	int ExtractFeatures(IplImage *img, IplImage *mask);
	int ExtractFeatures(ImgBgr &img, ImgBgr &mask);
	int ExtractFeatures(ImgBgr &img);

	void OverlayFeatures(ImgBgr *img);
	int DisplayMatches(ImgBgr &img, ImgBgr &img2, ImgBgr *display);
	int size() const {return m_size;}

	SURFDescriptor &operator()(int index){ return m_descriptors[index]; }
	SURFDescriptor operator()(int index) const { return m_descriptors[index]; }
	SURFDescriptor *operator[](int index) const { return &(m_descriptors[index]); }
	bool extended() const { return m_extended; }
	
	//Steve did this, Peasley does not agree, but it's going to be okay
	int PutativeMatch(const SURF &surf2, int prune_dist, float percent = 0.1f);

	//Bryan added this matching function, similar to SIFT matching
	//returns # of matches found
	int StandardMatch(const SURF &surf2, float percent = 0.49f);

private:
	double 		m_threshold;
	bool		m_extended;
	SURFDescriptor	*m_descriptors;
	CvSeq 		*m_CvSeqKeypoints, *m_CvSeqDescriptors;
	CvMemStorage* m_storage;
	int		m_size;
};

};
#endif _BLEPO_SURF_H__