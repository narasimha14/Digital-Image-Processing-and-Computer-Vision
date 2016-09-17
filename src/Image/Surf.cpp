#include "Surf.h"

#define SQR(X) ((X) * (X))

namespace blepo {

inline float SAD_Descriptors(const SURFDescriptor &desc1, const SURFDescriptor &desc2, int length){
	float SAD = 0;
	float *p1 = desc1.descriptor, *p2 = desc2.descriptor;
	for(int i = 0; i < length; i++)
		SAD += fabs( *p1++ - *p2++ );
	
	return SAD;
}

inline float SSD_Descriptors(const SURFDescriptor &desc1, const SURFDescriptor &desc2, int length){
	float SSD = 0;
	float *p1 = desc1.descriptor, *p2 = desc2.descriptor;
	for(int i = 0; i < length; i++)
		SSD += SQR( *p1++ - *p2++ );
	
	return SSD;
}

int SURF::ExtractFeatures(IplImage *img, IplImage *mask){
	m_storage = cvCreateMemStorage(0);
	CvMat *gray_img = cvCreateMat(img->height, img->width, CV_8UC1); //create matrix which is what CV SURF operates on
	CvMat *gray_mask = cvCreateMat(mask->height, mask->width, CV_8UC1);
	cvCvtColor(img, gray_img, CV_BGR2GRAY); //convert color IplImage to a gray scale matrix
	cvCvtColor(mask, gray_mask, CV_BGR2GRAY);
	
	//Find surf features and calculate their descriptors
	CvSURFParams params = cvSURFParams(m_threshold, m_extended ? 1 : 0);		
	cvExtractSURF(gray_img, gray_mask, &m_CvSeqKeypoints, &m_CvSeqDescriptors, m_storage, params );

	m_descriptors = (SURFDescriptor *)malloc(m_CvSeqDescriptors->total*sizeof(SURFDescriptor));
			
	CvSURFPoint *r;
	SURFDescriptor *desc = m_descriptors;
	for(int i = 0; i < m_CvSeqKeypoints->total; i++){
		r = (CvSURFPoint*)cvGetSeqElem( m_CvSeqKeypoints, i );
		
		desc->x = r->pt.x;
		desc->y = r->pt.y;
		desc->radius = (float)r->size;

		desc->descriptor =	(float *)cvGetSeqElem( m_CvSeqDescriptors, i);
		desc++;		
	}
	m_size = m_CvSeqDescriptors->total;
	cvReleaseMat(&gray_img);
	return m_CvSeqDescriptors->total;
}

int SURF::ExtractFeatures(ImgBgr &img) {
		ImgIplImage tmp(img);
		ImgBgr mask;
		mask.Reset(img.Width(), img.Height());
		Set(&mask,Bgr(255,255,255));
		ImgIplImage tmp_mask(mask);
		return ExtractFeatures(tmp.operator IplImage *(),tmp_mask.operator IplImage *());
};

int SURF::ExtractFeatures(ImgBgr &img, ImgBgr &mask) {
		ImgIplImage tmp(img);
		ImgIplImage tmp_mask(mask);
		return ExtractFeatures(tmp.operator IplImage *(),tmp_mask.operator IplImage *());
};

void SURF::OverlayFeatures(ImgBgr *img){ //overlay features onto an image
	Point center;
	int radius;

	SURFDescriptor *desc = m_descriptors;
	for(int i = 0; i < m_size; i++){
		center.x = cvRound(desc->x);
		center.y = cvRound(desc->y);
		radius = cvRound(desc->radius*1.2/9.*2);
		DrawCircle(center,radius,img, Bgr::RED,1);
		desc++;
	}	
}

int SURF::PutativeMatch(const SURF &surf2, int prune_dist, float percent) {
	vector<int> 	index1(m_size);
	vector<float>	minval1(m_size);
	
	vector<int> 	index2(surf2.size());
	vector<float>	minval2(surf2.size());

	float val;
	//for every feature in surf1, compare descriptor to every feature in surf2 and save the index
	//of the minimum SAD.
	vector<float>::iterator min1 = minval1.begin(), min2 = minval2.begin();
	vector<int>::iterator in1 = index1.begin(), in2 = index2.begin();
	SURFDescriptor *desc1 = m_descriptors;
	for(unsigned int i = 0; i < minval2.size(); i++)
		*min2++ = 999.0;

	int tempcnt,ext=0;
	while(1)
	{
		tempcnt=0;
		min1 = minval1.begin();
		in1 = index1.begin();
		for(int i = 0; i < m_size; i++){ 
			*min1 = 999.0;
			SURF::Iterator desc2 = surf2.Begin();
			min2 = minval2.begin();
			in2 = index2.begin();
			for(int j = 0; j < surf2.size(); j++) {
				if(sqrt(SQR(desc1->x-desc2->x) + SQR(desc1->y-desc2->y)) <= (prune_dist+ext))
					tempcnt++;
				val = SAD_Descriptors( *desc1, *desc2, m_extended ? 128 : 64 );	
			
				//comparing ith point in surf1 to every point in surf2
				if(val < *min1){
					*min1 = val;
					*in1 = j;
				}

				//comparing jth point in surf2 to ith point in surf1
				if(val < *min2){
					*min2 = val;
					*in2 = i;
				}
				desc2++;
				in2++;
				min2++;
			}
			desc1++;
			in1++;
			min1++;
		}
		if(tempcnt==0)
			ext++;
		else break;
	}

	//sort minvals to get top percent
	int minIndex;
	float temp;
	float avg = 0;
	min1 = minval1.begin();
	//find the average of minval1
	while(min1 != minval1.end())
		avg += *min1++;
	avg /= minval1.size();
	assert(percent > 0 && percent < 1);
	int endSort = (int)(percent*minval1.size());
	min1 = minval1.begin();
	for(int i = 0; i < endSort; i++) {
		minIndex = i;
		float currMin = *min1;
		vector<float>::const_iterator tmpMin = (minval1.begin()+i+1);
		for(unsigned int j = i+1; j < minval1.size(); j++) {
			if(*tmpMin < currMin) {
				minIndex = j;
				currMin =*tmpMin;
			}
			tmpMin++;
		}
		if(minIndex != i) {
			temp = *min1;
			*min1 = currMin;
			minval1[minIndex] = temp;
		}
		min1++;
	}

	float T = minval1[endSort];
	//see if minimum comparison is putative between feature point sets
	//dirty code, a little bit better now
	desc1 = m_descriptors;
	in1 = index1.begin();
	int totalCnt=0;
	for(unsigned int i = 0; i < index1.size(); i++)
	{
		if(index2[*in1] == i && minval2[*in1] < T)
		{
			desc1->match = surf2[*in1];
			//desc1->valid = true;
			//desc1->match->valid = true;
			totalCnt++;
		}
		else
		{
			desc1->match = NULL;
			//desc1->valid = false;
			//desc1->match->valid = false;
		}
		in1++;
		desc1++;
	}
	return totalCnt;
}

int SURF::StandardMatch(const SURF &surf2, float percent) {
	vector<int> 	index(2);
	vector<float>	minval(2);

	float val;
	SURFDescriptor *desc1 = m_descriptors;

	int tempcnt=0;
	for(int i = 0; i < m_size; i++)
	{ 
		minval[0] = 9999.0;
		minval[1] = 9999.0;
		index[0] = 0;
		index[1] = 0;
		SURF::Iterator desc2 = surf2.Begin();
		for(int j = 0; j < surf2.size(); j++)
		{
			val = SSD_Descriptors( *desc1, *desc2, m_extended ? 128 : 64 );	
			
			//comparing ith point in surf1 to every point in surf2
			if(val < minval[1])
			{
				if(val < minval[0])
				{
					minval[0] = val;
					index[0] = j;
				}
				else
				{
					minval[1] = val;
					index[1] = j;
				}
			}
			desc2++;
		}
		if(minval[0] < minval[1] * percent)
		{
			desc1->match = surf2[index[0]];
			tempcnt++;
		}
		else desc1->match = NULL;
		desc1++;
	}
	return tempcnt;
}

int SURF::DisplayMatches(ImgBgr &img, ImgBgr &img2, ImgBgr *display) {
	display->Reset(img.Width()*2,img.Height());
	ImgBgr::Iterator pDisp = display->Begin();
	ImgBgr::ConstIterator p1 = img.Begin();
	ImgBgr::ConstIterator p2 = img2.Begin();
	for(int j = 0; j < display->Height(); j++) {
		for(int i = 0; i < display->Width(); i++) {
			if(i < img.Width())
				*pDisp++ = *p1++;
			else
				*pDisp++ = *p2++;
		}
	}
	int matches=0;
	SURFDescriptor *desc = m_descriptors;
	int final = m_size;
	for(int i=0; i<final; i+=2)
	{
		SURFDescriptor *tmp = desc->match;
		if(tmp != NULL) {
		//if(tmp != NULL && desc->valid && tmp->valid) {
				Point p1 = Point((int)desc->x,(int)desc->y), p2 = Point((int)tmp->x+img.Width(),(int)tmp->y);
				DrawDot(p1,display,Bgr(0,255,0),3);
				DrawDot(p2,display,Bgr(0,255,0),3);
				DrawLine(p1,p2,display,Bgr::BLUE,1);
				matches++;
		}
		desc++;
	}
	return(matches);
}
};