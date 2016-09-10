/*
 * Copyright (c) 2011-2016 United States Government as represented by
 * the National Aeronautics and Space Administration.  No copyright
 * is claimed in the United States under Title 17, U.S.Code. All Other
 * Rights Reserved.
 */
#include "SimplePoly.h"
#include "Position.h"
#include "Vect2.h"
#include "VectFuns.h"
#include "GreatCircle.h"
#include "NavPoint.h"
#include "EuclideanProjection.h"
#include "Projection.h"
#include <string>
#include <vector>
/**
 * A basic polygon that describes a volume.  This volume has a flat bottom and top 
 * (specified as altitude values).  Points describe the cross-section area vertices in
 * a consistently clockwise (or consistently counter-clockwise) manner.  The cross-section
 * need not be convex, but an "out of order" listing of the vertices will result in edges 
 * that cross, and will cause several calculations to fail (with no warning).
 * 
 * A SimplePoly sets the altitude for all its points to be the _bottom_ altitude,
 * while the top is stored elsewhere as a single value.  The exact position for "top"
 * vertices is computed on demand.
 * 
 * Note: polygon support is experimental and the interface is subject to change!
 *
 */
namespace larcfm {

using std::vector;
using std::string;
using std::endl;

SimplePoly::SimplePoly() {
	bottomTopSet = false;
	top = 0;
	bottom = 0;
	points.reserve(6);
	boundingCircleDefined = false;
	centroidDefined = false;
	averagePointDefined = false;
	maxRad = -1.0;
	bRad = -1.0;
	cPos = Position::ZERO_LL();
}

SimplePoly::SimplePoly(double b, double t) {
	bottomTopSet = true;
	top = t;
	bottom = b;
	points.reserve(6);
	boundingCircleDefined = false;
	centroidDefined = false;
	averagePointDefined = false;
	maxRad = -1.0;
	bRad = -1.0;
	cPos = Position::ZERO_LL();
}

SimplePoly::SimplePoly(double b, double t, const std::string& units) {
	bottomTopSet = true;
	top = Units::from(units,t);
	bottom = Units::from(units,b);
	points.reserve(6);
	boundingCircleDefined = false;
	centroidDefined = false;
	averagePointDefined = false;
	maxRad = -1.0;
	bRad = -1.0;
	cPos = Position::ZERO_LL();
}


SimplePoly::SimplePoly(const SimplePoly& p) {
	bottomTopSet = p.bottomTopSet;
	bottom = p.bottom;
	top = p.top;
	boundingCircleDefined = false;
	centroidDefined = false;
	averagePointDefined = false;
	bPos = p.bPos;
	cPos = p.cPos;
	aPos = p.aPos;
	maxRad = p.maxRad;
	bRad = p.bRad;

	for(int i = 0; i < p.size(); i++) {
		addVertex(p.getVertex(i));
	}
}

bool SimplePoly::equals(const SimplePoly& p) const {
	bool ret = bottom == p.bottom && top == p.top;
	if (size() != p.size()) return false;
	for (int i = 0; i < size(); i++) {
		if (getVertex(i) != p.getVertex(i)) return false;
	}
	return ret;
}

// this uses euclidean coordinates
SimplePoly SimplePoly::make(const Poly3D& p3) {
	SimplePoly sp = SimplePoly();
	sp.setBottom(p3.getBottom());
	sp.setTop(p3.getTop());
	for(int i = 0; i < p3.size(); i++) {
		Position v = Position(Vect3(p3.getVertex(i),p3.getBottom()));
		sp.addVertex(v);
	}
	return sp;
}


/**
 * Create a SimplePoly from a Poly3D.  This SimplePoly will use latlon coordinates if proj != null, otherwise it will use Euclidean coordinates.
 */
SimplePoly SimplePoly::make(const Poly3D& p3, const EuclideanProjection& proj) {
	SimplePoly sp = SimplePoly();
	sp.setBottom(p3.getBottom());
	sp.setTop(p3.getTop());
	for(int i = 0; i < p3.size(); i++) {
		Position v = Position(Vect3(p3.getVertex(i),p3.getBottom()));
		//if (proj != NULL) {
		v = Position(proj.inverse(p3.getVertex(i), p3.getBottom()));
		//}
		sp.addVertex(v);
	}
	return sp;
}


bool SimplePoly::isLatLon() const {
	return points.size() > 0 && points[0].isLatLon();
}

int SimplePoly::size() const {
	return points.size();
}

// area and centroid courtesy of Paul Bourke (1988) http://paulbourke.net/geometry/polyarea/
// these are for non self-intersecting polygons
// this calculation assumes that point 0 = point n (or the start point is counted twice)
double SimplePoly::signedArea(double dx, double dy) const {
	double a = 0;
	int sz = points.size()-1;
	double xorig = points[0].x();
	for (int i = 0; i <= sz; i++) {
		double x0 = points[i].x();
		double x1 = (i==sz ? points[0].x() : points[i+1].x()) - dx;
		double y1 = (i==sz ? points[0].y() : points[i+1].y()) - dy;
		// correct for date line wrap around
		if (isLatLon()) {
			if (x0-xorig > Pi) {
				x0 = x0 - 2*Pi;
			} else if (x0-xorig < -Pi) {
				x0 = x0 + 2*Pi;
			}
			if (x1-xorig > Pi) {
				x1 = x1 - 2*Pi;
			} else if (x1-xorig < -Pi) {
				x1 = x1 + 2*Pi;
			}
		}
		a = a + (x0*y1 - x1*points[i].y());
	}
//	a = a + (points[sz].x()*points[0].y() - points[0].x()*points[sz].y()); // now do the 1st point again
	return 0.5*a;
}


void SimplePoly::calcCentroid() const {
	centroidDefined = true;
	Position bbcent = getBoundingRectangle().centerPos();
	double dx = bbcent.x();
	double dy = bbcent.y();
	double a = signedArea(dx, dy);
	// if a point or line, use old method
	if (a == 0) {
		Vect2 v2 = Vect2::ZERO;
		for (int i = 0; i < (signed)points.size(); i++) {
			v2 = v2.Add(points[i].vect2());
		}
		v2 = v2.Scal(1.0/points.size());
		double z = (top+bottom)/2.0;
		if (points[0].isLatLon()) {
			LatLonAlt lla = LatLonAlt::mk(v2.y, v2.x, z);
			cPos = Position(lla);
		} else {
			cPos = Position(Vect3(v2,z));
		}
	} else {
		// Paul's calculation
		double x = 0;
		double y = 0;
		double xorig = points[0].x();
		int sz = points.size()-1;
		for (int i = 0; i <= sz; i++) {
			double x0 = points[i].x() - dx;
			double x1 = (i == sz ? points[0].x() : points[i+1].x()) -dx;
			double y0 = points[i].y() - dy;
			double y1 = (i == sz ? points[0].y() : points[i+1].y()) - dy;
			// correct for date line wrap around
			if (isLatLon()) {
				if (x0-xorig > Pi) {
					x0 = x0 - 2*Pi;
				} else if (x0-xorig < -Pi) {
					x0 = x0 + 2*Pi;
				}
				if (x1-xorig > Pi) {
					x1 = x1 - 2*Pi;
				} else if (x1-xorig < -Pi) {
					x1 = x1 + 2*Pi;
				}
			}
			x = x + (x0+x1)*(x0*y1-x1*y0);
			y = y + (y0+y1)*(x0*y1-x1*y0);
		}
//		// now the last and first
//		double x0 = points[sz].x();
//		double x1 = points[0].x();
//		double y0 = points[sz].y();
//		double y1 = points[0].y();
//		x = x + (x0+x1)*(x0*y1-x1*y0);
//		y = y + (y0+y1)*(x0*y1-x1*y0);

		x = x/(6*a) + dx;
		y = y/(6*a) + dy;

		cPos = points[0].mkX(x).mkY(y).mkZ((top+bottom)/2);
		if (!getBoundingRectangle().contains(cPos)) {
			// in latlon with small polygons this sometimes returns an incorrect value
//			f.pln("WARNING SimplePoly.calcCentroid has encountered a numeric error.  Returning averagePoint instead. "+boundingCircleRadius());
//			cPos = averagePoint();
			cPos = getBoundingRectangle().centerPos();
		}
	}
}

// if we have a more accurate bounding circle than one based on the centroid
//void SimplePoly::calcBoundingCircleCenter() const {
//	boundingCircleDefined = true;
//	double maxD = 0.0;
//	Position p1 = points[0];
//	Position p2 = points[0];
//	for (int i = 0; i < (signed)points.size(); i++) {
//		Position p3 = points[i];
//		Position p4 = maxDistPair(points[i]);
//		double d = p3.distanceH(p4);
//		if (d > maxD) {
//			maxD = d;
//			p1 = p3;
//			p2 = p4;
//		}
//	}
//	double r = maxD/2;
//	double f = 0.5;
//	if (p1.isLatLon()) {
//		bPos = Position(GreatCircle::interpolate(p1.lla(),p2.lla(),f));
//	} else {
//		NavPoint np1 = NavPoint(p1,0);
//		NavPoint np2 = NavPoint(p2,1);
//		bPos = np1.linear(np1.initialVelocity(np2), 0.5).position();
//	}
//
//	// now move along the segment to the farthest from  the current cPos
//	p2 = maxDistPair(bPos);
//	double d = p2.distanceH(bPos);
//	if (d > r) {
//		f = (d - r)/d;	// fraction of distance along new segment
//		if (p2.isLatLon()) {
//			bPos = Position(GreatCircle::interpolate(bPos.lla(),p2.lla(),f));
//		} else {
//			NavPoint np1 = NavPoint(bPos,0);
//			NavPoint np2 = NavPoint(p2,1);
//			bPos = np1.linear(np1.initialVelocity(np2), 0.5).position();
//		}
//	}
//}


Position SimplePoly::centroid() const {
	if (!centroidDefined) {
		calcCentroid();
	}
	return cPos;
}

// note: this may fail over poles!
Position SimplePoly::averagePoint() const {
	if (!averagePointDefined) {
		Position bbcent = getBoundingRectangle().centerPos();
		double dx = bbcent.x();
		double dy = bbcent.y();
		double x = 0;
		double y = 0;
		double xorig = points[0].x()-dx;
		for (int i = 0; i < (int) points.size(); i++) {
			double x0 = points[i].x()-dx;
			double y0 = points[i].y()-dy;
			// correct for date line wrap around
			if (isLatLon()) {
				if (x0-xorig > Pi) {
					x0 = x0 - 2*Pi;
				} else if (x0-xorig < -Pi) {
					x0 = x0 + 2*Pi;
				}
			}
			x += x0;
			y += y0;
		}
		aPos = points[0].mkX(dx+x/points.size()).mkY(dy+y/points.size()).mkZ((top+bottom)/2);
		averagePointDefined = true;
	}
	return aPos;
}


// returns the center of a bounding circle
// calculates it and radius, if necessary
Position SimplePoly::boundingCircleCenter() const {
	return centroid();
}

/** Returns true if this polygon is convex */
bool SimplePoly::isConvex() {
	bool ret = true;
	if (points.size() > 2) {
		double a1 = points[points.size()-1].track(points[0]);
		double a2 = points[0].track(points[1]);
		int pdir = Util::turnDir(a1, a2);
		for (int i = 0; i < (int) points.size()-2; i++) {
			a1 = a2;
			a2 = points[i+1].track(points[i+2]);
			int dir = Util::turnDir(a1, a2);
			ret = ret && (pdir == 0 || dir == pdir);
			if (dir != 0) pdir = dir;
		}
	}
	return ret;
}


double SimplePoly::boundingCircleRadius() const {
	return maxRadius();
}

Position SimplePoly::maxDistPair(const Position& p) const {
	double maxD = 0.0;
	Position p2 = p;
	for (int i = 0; i < (signed)points.size(); i++) {
		double d = points[i].distanceH(p);
		if (d > maxD) {
			p2 = points[i];
			maxD = d;
		}
	}
	return p2;
}



double SimplePoly::apBoundingRadius() {
	if (bRad <= 0.0) {
		Position c = averagePoint();
		Position p = maxDistPair(c);
		bRad = c.distanceH(p);
	}
	return bRad;
}


bool SimplePoly::addVertex(const Position& p) {
	// error checking
	if (p.isInvalid()) {
		return false;
	}
	//	 for (int i = 0; i < (int) points.size(); i++) {
	//		 if (points[i] == p) return false;
	//	 }

	centroidDefined = false;
	boundingCircleDefined = false;
	averagePointDefined = false;
	maxRad = -1.0;
	bRad = -1.0;
	if (points.size() < 1 && !bottomTopSet) {
		top = bottom = p.alt();
		bottomTopSet = true;
	}
	points.push_back(p.mkAlt(bottom));
	return true;
}


// return the max horizontal distance between a vertex and the centroid
double SimplePoly::maxRadius() const {
	if (maxRad <= 0.0) {
		Position c = centroid();
		Position p = maxDistPair(c);
		maxRad = c.distanceH(p);
	}
	return maxRad;
}



void SimplePoly::remove(int n) {
	centroidDefined = false;
	boundingCircleDefined = false;
	averagePointDefined = false;
	maxRad = -1.0;
	bRad = -1.0;

	if (n >= 0 && n < (signed)points.size()) {
		points.erase(points.begin()+n);
	}
}

/** This currently does NOT set the Z component of the point (unless it is the first point) */
bool SimplePoly::setVertex(int n, Position p) {
	// error checking
	if (p.isInvalid()) {
		return false;
	}
	Position pb = p.mkAlt(bottom);
	for (int i = 0; i < (int) points.size(); i++) {
		if (points[i] == pb) return false;
	}

	centroidDefined = false;
	boundingCircleDefined = false;
	averagePointDefined = false;
	maxRad = -1.0;
	bRad = -1.0;
	if (n >= 0 && n < (signed)points.size()) {
		points[n] = pb;
	}
	return true;
}


void SimplePoly::setTop(double t) {
	centroidDefined = false;
	boundingCircleDefined = false;
	averagePointDefined = false;
	maxRad = -1.0;
	bRad = -1.0;
	bottomTopSet = true;
	top = t;
}

double SimplePoly::getTop() const {
	return top;
}

void SimplePoly::setBottom(double b) {
	centroidDefined = false;
	boundingCircleDefined = false;
	averagePointDefined = false;
	maxRad = -1.0;
	bRad = -1.0;
	bottomTopSet = true;
	bottom = b;
	for (int i = 0; i < (signed)points.size(); i++) {
		points[i] = points[i].mkAlt(bottom);
	}
}

double SimplePoly::getBottom() const {
	return bottom;
}

// on invalid n, this returns the centroid!
Position SimplePoly::getVertex(int n) const {
	if (n >= 0 && n < (signed)points.size()) {
		return points[n];
	}
	return centroid();
}

// on invalid n, this returns the centroid!
Position SimplePoly::getTopPoint(int n) const {
	if (n >= 0 && n < (signed)points.size()) {
		return points[n].mkAlt(top);
	}
	return centroid();
}

SimplePoly SimplePoly::copy() const {
	SimplePoly r = SimplePoly(bottom,top);
	for(int i = 0; i < (signed)points.size(); i++) {
		r.addVertex(points[i]);
	}
	return r;
}

/** return a aPolygon3D version of this */
Poly3D SimplePoly::poly3D(const EuclideanProjection& proj) const {
	Poly3D p3;
	if (isLatLon()) {
		p3 = Poly3D();
		for (int i = 0; i < (int) points.size(); i++) {
			Position p = points[i];
			p3.addVertex(proj.project(p).vect2());
		}
	} else {
		p3 = Poly3D();
		for (int i = 0; i < (int) points.size(); i++) {
			Position p = points[i];
			p3.addVertex(p.point().vect2());
		}
	}
	p3.setBottom(getBottom());
	p3.setTop(getTop());
	return p3;
}

bool SimplePoly::contains(const Position& p) const {
	EuclideanProjection proj = Projection::createProjection(p);
	Poly3D poly = poly3D(proj);
	if (p.isLatLon()) {
		return poly.contains(Vect3::ZERO);
	} else {
		return poly.contains(p.point());
	}

}

bool SimplePoly::contains2D(const Position& p) const {
	EuclideanProjection proj = Projection::createProjection(p);
	Poly2D poly = poly3D(proj).poly2D();
	if (p.isLatLon()) {
		return poly.contains(Vect2::ZERO);
	} else {
		return poly.contains(p.vect2());
	}
}




/**
 * This moves the SimplePoly by the amount determined by the given (Euclidean) offset.
 * @param off offset
 */
void SimplePoly::translate(const Vect3& off) {
	Position c = centroid();
	if (isLatLon()) {
		EuclideanProjection proj = Projection::createProjection(c.lla());
		for (int i = 0; i < size(); i++) {
			setVertex(i, Position(proj.project(points[i])));
		}
	}
	// translate
	for (int i = 0; i < size(); i++) {
		Position p = points[i];
		setVertex(i, p.mkX(p.x()+off.x).mkY(p.y()+off.y));
	}
	// shift back to latlon, if necessary
	if (isLatLon()) {
		EuclideanProjection proj = Projection::createProjection(c.lla());
		for (int i = 0; i < size(); i++) {
			setVertex(i, Position(proj.inverse(points[i].point())));
		}
	}
	setTop(top+off.z);
	setBottom(bottom+off.z);
}

SimplePoly SimplePoly::linear(const Velocity& v, double t) const {
	SimplePoly newPoly;
	int sz = size();
	for (int j = 0; j < sz; j++) {
		Position p = getVertex(j).linear(v,t);
		Position pt = getTopPoint(j).linear(v,t);
		newPoly.addVertex(p);
		newPoly.setBottom(p.z());
		newPoly.setTop(pt.z());
	}
	return newPoly;
}

bool SimplePoly::validate() {
	// not a polygon
	if (size() < 3)	return false;
	for (int i = 0; i < (int) points.size(); i++) {
		// invalid points
		if (points[i].isInvalid()) return false;
		int j;
		for (j = 0; j < i; j++) {
			// duplicated points
			if (points[i].distanceH(points[j]) < Constants::get_horizontal_accuracy()) return false;
			if (j < i-2) {
				// check for intersections
				if (isLatLon()) {
					LatLonAlt a = points[j].lla();
					LatLonAlt b = points[j+1].lla();
					LatLonAlt c = points[j-1].lla();
					LatLonAlt d = points[i].lla();
					double t = GreatCircle::intersection(a, b, 100.0, c, d).second;
					if (t >= 0.0 && t <= 100.0) return false;
				} else {
					Vect3 a = points[j].point();
					Vect3 b = points[j+1].point();
					Vect3 c = points[j-1].point();
					Vect3 d = points[i].point();
					double t = VectFuns::intersection(a, b, 100.0, c, d).second;
					if (t >= 0.0 && t <= 100.0) return false;
				}
			}
		}
		if (i > 1) {
			// redundant (consecutive collinear) points
			if (isLatLon()) {
				if (GreatCircle::collinear(points[i-2].lla(), points[j-1].lla(), points[i].lla())) return false;
			} else {
				if (VectFuns::collinear(points[i-2].vect2(), points[j-1].vect2(), points[i].vect2())) return false;
			}
		}
	}
	return true;
}

BoundingRectangle SimplePoly::getBoundingRectangle() const {
	BoundingRectangle br;
	for (int i =  0; i < (int) points.size(); i++) {
		br.add(points[i]);
	}
	return br;
}


string SimplePoly::toString() const {
	string s = "";
	for (int i = 0; i < size(); i++) {
		s = s + getVertex(i).toString()+" ";
	}
	return s;
}

} // namespace 
