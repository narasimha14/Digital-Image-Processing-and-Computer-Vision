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
#ifndef __BLEPO_POINTCLOUD_H__
#define __BLEPO_POINTCLOUD_H__

#include <Windows.h>
#include <vector>
#include "../Image/Image.h"
#include <fstream>
#include <iostream>
#include <string>
#include <stdio.h>  // fopen_s

/////////////////////////////////////////////////////////
//This file contains information about the point cloud class
using namespace std;

namespace blepo
{
  //ColoredPoint class for 3D color information
  class ColoredPoint
  {
  public:
    float x, y, z;
    Bgr color;
    bool valid;

    ColoredPoint() : x(0), y(0), z(0), color(0,0,0) { }
    ColoredPoint(float xx, float yy, float zz, const Bgr& c, bool use=true) : x(xx), y(yy), z(zz), color(c), valid(use) { }
    
    inline void operator=(const ColoredPoint& other) 
    {
      x = other.x;
      y = other.y;
      z = other.z;
      color = other.color;
	  valid = other.valid;
    }
  };
  
  class PointCloud 
  {
  private:
    vector<ColoredPoint> m_data;
	int m_width;
	ColoredPoint m_max;
    ColoredPoint m_min;

  public:
    typedef vector<ColoredPoint>::iterator Iterator;
    typedef vector<ColoredPoint>::const_iterator ConstIterator;
    
    PointCloud() {
      m_width = 640;
    }
    
    ~PointCloud() {
      
    }
    
    void ReleaseData() {
      m_data.clear();
    }
    
    inline void Add(const ColoredPoint &other) {
      m_data.push_back(other);
    }
    
    inline const ColoredPoint operator[] (const int index) {
      
      return m_data[index];
    }

	inline const ColoredPoint operator()(int x, int y) {
		return m_data[y*m_width+x];
	}

	inline ColoredPoint Min() {
		return m_min;
	}

	inline ColoredPoint Max() {
		return m_max;
	}
    
    inline Iterator Begin() {
      return m_data.begin();
    }
    
    inline Iterator End() {
      return m_data.end();
    }
    
    inline ConstIterator Begin() const {
      return m_data.begin();
    }
    
    inline ConstIterator End() const {
      return m_data.end();
    }
    
    inline int Size() const {
      return m_data.size();
    }

	inline void Resize(const int size) {
		m_data.resize(size);
	}
	/* Kinect Specific
	inline ColoredPoint GetPointFromIndices(int x, int y, float z, blepo::Bgr color) {
		return ColoredPoint(float((x - CX_D) * z * FX_D),float((479 - (y - CY_D)) * z * FY_D),z,color);
	}

	inline ColoredPoint GetPointFromIndicesNoDepth(int x, int y, blepo::Bgr color) {
		return ColoredPoint(float((x - CX_D) * FX_D),float((479 - (y - CY_D)) * FY_D),0,color);
	}
	*/
	inline void Set(const ColoredPoint &val)
	{
		for (Iterator p = Begin() ; p != End() ; p++)  *p = val;
	}

	inline void Set(const ColoredPoint &val, int index) {
		m_data[index] = val;
	}

	void operator=(PointCloud &rhs) {
		m_width = rhs.Size();
		m_data.clear();
		for(int i = 0; i < m_width; i++)
			m_data.push_back(rhs[i]);
	};
    
    void SaveCloudDataToPly(const char *fileName) const
    {
      FILE *file = NULL;
//      fopen_s(&file, fileName, "w");
      file = fopen(fileName, "w");
      if(file == NULL) 
      {
        BLEPO_ERROR("Error, cannot open file");
      } 
      else 
      {
        fprintf(file,
          "ply\n"
          "format ascii 1.0\n"
          "element vertex %d\n"
          "property float x\n"
          "property float y\n"
          "property float z\n"
          "property uchar diffuse_red\n"
          "property uchar diffuse_green\n"
          "property uchar diffuse_blue\n"
          "end_header\n", 
          m_data.size());
        ConstIterator p = m_data.begin();
        while(p != m_data.end()) 
        {
          fprintf(file,"%lf %lf %lf %d %d %d\n", p->x, p->y, p->z, p->color.r, p->color.g, p->color.b);
          p++;
        }
        fclose(file);
      }
    }

    void LoadCloudFromPly(const CString& fileName) 
    {
      ifstream data;
      data.open(fileName);
      if(!data.is_open()) 
      {
        BLEPO_ERROR("Error, cannot open file");
      } 
      else 
      {
        string dont_care;
        int num_vertices = 0;

        // read header
        int num_properties = 0;
        string property_list;
        do {
          getline(data, dont_care);
          if(data.eof())
          {
            BLEPO_ERROR("Error processing pointCloud");
          }

          // read number of vertices
          const char str_ev[] = "element vertex ";
          const int ev_len = strlen(str_ev);
          if (dont_care.compare(0, ev_len, "element vertex ") == 0) 
          {
            const char *tmp = dont_care.c_str();
            num_vertices = atoi(tmp + ev_len);
          }	
          // read number of vertices
          if (dont_care.compare(0, strlen("property"), "property") == 0)
          {
            char s1[1000], s2[1000], s3[1000];
            sscanf(dont_care.c_str(), "%s %s %s", s1, s2, s3);
            property_list += s3;
            property_list += ' ';
            num_properties++;
          }	

//          //printf_s("dont_care: %s\n",dont_care);
        } while (dont_care != "end_header");

        //AfxMessageBox(property_list);

        if (num_vertices == 0)
        {
          BLEPO_ERROR("Error, empty pointCloud");
        }

        // load rest of file
        float x,y,z;
        Bgr color;
        int i = 0;
        const int max_no_chars_rgb = 3;
        const int max_no_chars_xyz = 9;
		m_max=ColoredPoint(-100,-100,-100,Bgr(0,0,0));
		m_min=ColoredPoint(100,100,100,Bgr(0,0,0));
        do 
        { // this code is dumb; it only works for certain xyzrgb orders

          string str;
          getline(data, str);

          if (property_list == "x y z nx ny nz diffuse_red diffuse_green diffuse_blue ")
          {
            float t1, t2, t3;  // tmp
            int r, g, b;
            sscanf(str.c_str(), "%f %f %f %f %f %f %d %d %d", &x, &y, &z, &t1, &t2, &t3, &r, &g, &b); 
            color.r = r;
            color.g = g;
            color.b = b;
			if(x !=0)
			{
				//finding maximum x,y,z values
				if(m_max.x<x)
					m_max.x=x;
				if(m_max.y<y)
					m_max.y=y;
				if(m_max.z<z)
					m_max.z=z;
				//finding minimum x,y,z values
				if(m_min.x>x)
					m_min.x=x;
				if(m_min.y>y)
					m_min.y=y;
				if(z != 0 && m_min.z>z)
					m_min.z=z;
			}
          }
          else if (property_list == "x y z diffuse_red diffuse_green diffuse_blue ")
          {
            int r, g, b;
            sscanf(str.c_str(), "%f %f %f %d %d %d", &x, &y, &z, &r, &g, &b); 
            color.r = r;
            color.g = g;
            color.b = b;
			if(x !=0)
			{
				//finding maximum x,y,z values
				if(m_max.x<x)
					m_max.x=x;
				if(m_max.y<y)
					m_max.y=y;
				if(m_max.z<z)
					m_max.z=z;
				//finding minimum x,y,z values
				if(m_min.x>x)
					m_min.x=x;
				if(m_min.y>y)
					m_min.y=y;
				if(z != 0 && m_min.z>z)
					m_min.z=z;
			}
          }
          else
          {
            // If your PLY format was not recognized, should be easy to add another 'else if' clause to handle it
            BLEPO_ERROR("Unrecognized PLY format");
          }

          Add(ColoredPoint(x, y, z, color));
          i++;
        } while (!data.eof() && i < num_vertices);
        data.close();
      }
    }
    
    /*inline ColorPoint* Find(const ColorPoint &other) {
    return find(data.begin(),data.end(),other);
    }*/
    
  };
};  // end namespace blepo

#endif __BLEPO_POINTCLOUD_H__