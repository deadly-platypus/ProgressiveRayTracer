#include "QuadTreeNode.h"

#include <G3D/Vector2.h>


QuadTreeNode::QuadTreeNode(G3D::Point2 point)
{
	this->x = point.x;
	this->y = point.y;
	this->color = G3D::Color3::black();
}


QuadTreeNode::~QuadTreeNode(void)
{
}
