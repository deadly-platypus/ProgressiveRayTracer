#pragma once
#include <G3D/Vector2.h>
#include <G3D/Color3.h>

class QuadTreeNode
{
public:
	QuadTreeNode(G3D::Point2 point);
	~QuadTreeNode(void);

	float x, y;
	G3D::Color3 color;
};

