#pragma once
#include <G3D/Rect2D.h>
#include <G3D/Vector2.h>
#include <G3D/Color3.h>

#include <vector>

#include "QuadTreeNode.h"

class QuadTree
{
public:
	const static int MAX_POINTS = 5;
	const static int MIN_AREA	= 25;

	QuadTree(float x, float y, float width, float height);
	~QuadTree(void);

	std::vector<QuadTreeNode> points;
	bool insert(const G3D::Point2& point);
	int size();
	int point_count();
	
	QuadTree *nw;
	QuadTree *ne;
	QuadTree *sw;
	QuadTree *se;
	G3D::Rect2D *boundary;

	G3D::Color3 color;

	float neighborColorDiff;

	static int compare(const void *left, const void *right){
		QuadTree *q1 = (QuadTree*)left;
		QuadTree *q2 = (QuadTree*)right;

		return (q1->neighborColorDiff - q2->neighborColorDiff);
	}

private:
	void subdivide();
};

class QuadTreeDiffComparator {
public:
	bool operator()(const QuadTree *left, const QuadTree *right) {
		return (left->neighborColorDiff <= right->neighborColorDiff);
	}
};

class QuadTreeCenterComparator {
public:
	bool operator()(const QuadTree *left, const QuadTree *right) {
		return (left->boundary->center() != right->boundary->center());
	}
};

