/*
 * Copyright (c) 2004, Laminar Research.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include "WED_MapZoomerNew.h"
#include "GUI_Messages.h"
#include "XESConstants.h"

#include "CompGeomDefs3.h"
#include "glew.h"

inline	double	rescale(double s1, double s2, double d1, double d2, double v)
{
	return ((v - s1) * (d2 - d1) / (s2 - s1)) + d1;
}

WED_MapZoomerNew::WED_MapZoomerNew(WED_Camera * c)
	: cam(c),
	  mCacheKey(0),
	  mPixels{0, 0, 1, 1},
	  mLogicalBounds{-180, -90, 180, 90},
	  mLonCenter(0), mLatCenter(0),
	  mCenterX(0.5), mCenterY(0.5),
	  mPixel2DegLat(1.0)
{
	RecalcAspectRatio();
}

WED_MapZoomerNew::~WED_MapZoomerNew()
{
}

double	WED_MapZoomerNew::XPixelToLon(double x) const
{
	return mLonCenter + (x - mCenterX) * mPixel2DegLat / mLonCenterCOS;
}

double	WED_MapZoomerNew::YPixelToLat(double y) const
{
	return mLatCenter + (y - mCenterY) * mPixel2DegLat;
}

double	WED_MapZoomerNew::LonToXPixel(double lon) const
{
	return mCenterX + (lon - mLonCenter) * mLonCenterCOS / mPixel2DegLat;
}

double	WED_MapZoomerNew::LatToYPixel(double lat) const
{
	return mCenterY + (lat - mLatCenter) / mPixel2DegLat;
}

Point2	WED_MapZoomerNew::PixelToLL(const Point2& p) const
{
	return Point2(XPixelToLon(p.x()), YPixelToLat(p.y()));
}

Point2	WED_MapZoomerNew::LLToPixel(const Point2& p) const
{
	return Point2(LonToXPixel(p.x()), LatToYPixel(p.y()));
}

void	WED_MapZoomerNew::PixelToLLv(Point2 * dst, const Point2 * src, int n) const
{
	while(n--)
		*dst++ = PixelToLL(*src++);
}

void	WED_MapZoomerNew::LLToPixelv(Point2 * dst, const Point2 * src, int n) const
{
	while(n--)
		*dst++ = LLToPixel(*src++);
}

double	WED_MapZoomerNew::GetPPM(void) const
{
	#if BENTODO
	can we do better?
	// return fabs(LatToYPixel(MTR_TO_DEG_LAT) - LatToYPixel(0.0));
	#endif
	return MTR_TO_DEG_LAT / mPixel2DegLat;
}

double WED_MapZoomerNew::GetClickRadius(double p) const
{
	return p * mPixel2DegLat;
}


void	WED_MapZoomerNew::SetPixelBounds(
					double 	inLeft,
					double	inBottom,
					double	inRight,
					double	inTop)
{
	++mCacheKey;
	mPixels[0] = inLeft;           // there is some redundancy here ...
	mPixels[1] = inBottom;
	mPixels[2] = inRight;
	mPixels[3] = inTop;
	mCenterX = 0.5 * (inLeft + inRight);
	mCenterY = 0.5 * (inBottom + inTop);

	BroadcastMessage(GUI_SCROLL_CONTENT_SIZE_CHANGED,0);
}

void	WED_MapZoomerNew::SetPixelCenter(double x, double y)
{
	mCenterX = x;
	mCenterY = y;
}

void	WED_MapZoomerNew::SetMapLogicalBounds(
					double	inWest,
					double	inSouth,
					double	inEast,
					double	inNorth)
{
	++mCacheKey;
	mLogicalBounds[0] = inWest;
	mLogicalBounds[1] = inSouth;
	mLogicalBounds[2] = inEast;
	mLogicalBounds[3] = inNorth;

	mLonCenter = 0.5 * (inWest + inEast);
	mLatCenter = 0.5 * (inNorth + inSouth);
	mLonCenterCOS = cos(mLatCenter * DEG_TO_RAD);

	BroadcastMessage(GUI_SCROLL_CONTENT_SIZE_CHANGED,0);
}


void	WED_MapZoomerNew::GetPixelBounds(
					double& outLeft,
					double&	outBottom,
					double&	outRight,
					double&	outTop)
{
	outLeft = mPixels[0];
	outBottom = mPixels[1];
	outRight = mPixels[2];
	outTop = mPixels[3];
}

void	WED_MapZoomerNew::GetMapVisibleBounds(
					double&	outWest,
					double&	outSouth,
					double&	outEast,
					double&	outNorth)
{
	outWest = XPixelToLon	(mPixels[0]);
	outSouth = YPixelToLat	(mPixels[1]);
	outEast = XPixelToLon	(mPixels[2]);
	outNorth = YPixelToLat	(mPixels[3]);
}

void	WED_MapZoomerNew::GetMapLogicalBounds(
					double&	outWest,
					double&	outSouth,
					double&	outEast,
					double&	outNorth)
{
	outWest	= 	mLogicalBounds[0];
	outSouth=	mLogicalBounds[1];
	outEast	=	mLogicalBounds[2];
	outNorth=	mLogicalBounds[3];
}


//void	WED_MapZoomerNew::SetAspectRatio(double a)
//{
//	mAspectRatio = a;
//}

void	WED_MapZoomerNew::ZoomShowAll(void)
{
	ZoomShowArea(mLogicalBounds[0],mLogicalBounds[1],mLogicalBounds[2],mLogicalBounds[3]);
}

void	WED_MapZoomerNew::ZoomShowArea(
							double	inWest,
							double	inSouth,
							double	inEast,
							double	inNorth)
{
	++mCacheKey;
	mLonCenter = (inWest + inEast) * 0.5;
	mLatCenter = (inSouth + inNorth) * 0.5;

	double required_width_logical = inEast - inWest;
	double required_height_logical = inNorth - inSouth;

	if(required_width_logical == 0)
		required_width_logical = 0.00001;

	if(required_height_logical == 0)
		required_height_logical = 0.00001;

	double pix_avail_width = mPixels[2] - mPixels[0];
	double pix_avail_height = mPixels[3] - mPixels[1];

	double scale_for_vert = required_height_logical / pix_avail_height;
	double scale_for_horz = required_width_logical / pix_avail_width * mLonCenterCOS;

	mPixel2DegLat = max(scale_for_vert,scale_for_horz);
	RecalcAspectRatio();

	BroadcastMessage(GUI_SCROLL_CONTENT_SIZE_CHANGED,0);
}


void	WED_MapZoomerNew::PanPixels(
					double	x1,
					double	y1,
					double	x2,
					double	y2)
{
	++mCacheKey;

	// This is straight-forward: calculate the difference in lat/lon numbers between
	// the two pixels and change.  Positive numbers mean the user dragged up/right
	// therefore the map's bounds are decreasing!  Whacky, eh?
	double delta_lon = XPixelToLon(x2) - XPixelToLon(x1);
	double delta_lat = YPixelToLat(y2) - YPixelToLat(y1);

	mLatCenter -= delta_lat;
	mLonCenter -= delta_lon;
	RecalcAspectRatio();

	BroadcastMessage(GUI_SCROLL_CONTENT_SIZE_CHANGED,0);
}

void	WED_MapZoomerNew::ZoomAround(
					double	zoomFactor,
					double	centerXPixel,
					double	centerYPixel)
{
	++mCacheKey;

	// Zoom the map around a point.  We do this in three steps because I am lazy:
	// 1. Scroll the map so that we are zooming around the lower left corner.
	// 2. Zoom the map by adjusting only the top and right logical bounds, not
	//    the lower left.
	// 3. Scroll the map back.

	PanPixels(centerXPixel, centerYPixel, mCenterX, mCenterY);

	if (zoomFactor <= 1.0 || mPixel2DegLat > 1e-8) // limit manual zoom in to 1 mm/pixel (108,900 meter / deg lat)
		mPixel2DegLat /= zoomFactor;
	RecalcAspectRatio();

	PanPixels(mCenterX, mCenterY, centerXPixel, centerYPixel);
	BroadcastMessage(GUI_SCROLL_CONTENT_SIZE_CHANGED,0);
}

void	WED_MapZoomerNew::GetScrollBounds(float outTotalBounds[4], float outVisibleBounds[4])
{
	double vbounds[4];
	GetMapVisibleBounds(vbounds[0],vbounds[1],vbounds[2],vbounds[3]);

	for (int n = 0; n < 4; ++n)
	{
		outTotalBounds[n] = mLogicalBounds[n];
		outVisibleBounds[n] = vbounds[n];
	}
}

void	WED_MapZoomerNew::ScrollH(float xOffset)
{
	++mCacheKey;

	float log[4], vis[4];
	GetScrollBounds(log, vis);

	float now = vis[0] - log[0];
	xOffset -= now;
	mLonCenter += xOffset;// * mPixel2DegLat / mLonCenterCOS);
}

void	WED_MapZoomerNew::ScrollV(float yOffset)
{
	++mCacheKey;

	float log[4], vis[4];
	GetScrollBounds(log, vis);

	float now = vis[1] - log[1];
	yOffset -= now;
	mLatCenter += (yOffset);// * mPixel2DegLat);
	RecalcAspectRatio();
}

void	WED_MapZoomerNew::RecalcAspectRatio(void)
{
	++mCacheKey;

	double top_lat = YPixelToLat(mPixels[3]);
	double bot_lat = YPixelToLat(mPixels[1]);

	if (top_lat > 0 && bot_lat < 0)
		mLonCenterCOS = 1.0;
	else
		mLonCenterCOS = cos(min(fabs(top_lat),fabs(bot_lat)) * DEG_TO_RAD);
}

/********** new funcs for 3D preview / prespective projection *********/

double	WED_MapZoomerNew::PixelSize(const Bbox2& bboxLL) const
{
	Point2 p1 = LLToPixel(bboxLL.p1);
	Point2 p2 = LLToPixel(bboxLL.p2);

	if(cam)
		return cam->PixelSize(Bbox3(p1.x(), p1.y(), 0.0, p2.x(), p2.y(), 0.0));
	else
		return max(fabs(p1.x() - p2.x()), fabs(p1.y() - p2.y())) * GetPPM();
}

double	WED_MapZoomerNew::PixelSize(const Bbox2& bboxLL, double featureSize) const
{
	if(cam)
	{
		Point2 p1 = LLToPixel(bboxLL.p1);
		Point2 p2 = LLToPixel(bboxLL.p2);
		return cam->PixelSize(Bbox3(p1.x(), p1.y(), 0.0, p2.x(), p2.y(), 0.0), featureSize);
	}
	else
		return featureSize * GetPPM();
}

double	WED_MapZoomerNew::PixelSize(const Point2& positionLL, double diameter) const
{
	if(cam)
	{
		Point2 posPixel = LLToPixel(positionLL);
		return cam->PixelSize({posPixel.x(), posPixel.y(), 0.0}, diameter);
	}
	else
		return diameter * GetPPM();
}

void	WED_MapZoomerNew::PushMatrix(void)
{
	if(cam)
		cam->PushMatrix();
	else
		glPushMatrix();
}

void	WED_MapZoomerNew::Rotatef(float r, float x, float y, float z)
{
	if(cam)
		cam->Rotate(r, Vector3(x, y, z));
	else
		glRotatef(r, x, y, z);
}

void	WED_MapZoomerNew::Translatef(float x, float y, float z)
{
	if(cam)
		cam->Translate(Vector3(x, y, z));
	else
		glTranslatef(x, y, z);
}

void	WED_MapZoomerNew::Scalef(float x, float y, float z)
{
	if(cam)
		cam->Scale(x, y, z);
	else
		glScalef(x, y, z);
}

void	WED_MapZoomerNew::PopMatrix(void)
{
	if(cam)
		cam->PopMatrix();
	else
		glPopMatrix();
}

void	WED_MapZoomerNew::SetPPM(double ppm)
{
	if(ppm > 0.0)
		mPixel2DegLat = MTR_TO_DEG_LAT / ppm;
}
