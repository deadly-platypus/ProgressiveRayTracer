#include "QuadTree.h"
#include "QuadTreeNode.h"
#include <stdio.h>


QuadTree::QuadTree(float x, float y, float width, float height)
{
	this->points = std::vector<QuadTreeNode>();

	this->boundary = new G3D::Rect2D(G3D::Rect2D::xyxy(G3D::Point2(x, y), G3D::Point2(x + width, y + height)));

	nw = NULL;
	ne = NULL;
	sw = NULL;
	se = NULL;

	this->neighborColorDiff = 0.0f;
}

bool QuadTree::insert(const G3D::Point2& point){
	if(!this->boundary->contains(point)){
		return false;
	}

	for(int i = 0; i < this->points.size(); i++){
		if((int)this->points[i].x == (int)point.x && (int)this->points[i].y == (int)point.y){
			return false;
		}
	}

	if(this->points.size() < QuadTree::MAX_POINTS || this->boundary->area() <= QuadTree::MIN_AREA){
		this->points.push_back(QuadTreeNode(point));
		return true;
	}

	if(!nw){
		subdivide();
	}

	if(nw->insert(point)) return true;
	if(ne->insert(point)) return true;
	if(sw->insert(point)) return true;
	if(se->insert(point)) return true;

	return false;
}

void QuadTree::subdivide(){
	if(!this->nw) {
		this->nw = new QuadTree(this->boundary->center().x - this->boundary->width() * 0.5, this->boundary->center().y - this->boundary->height() * 0.5, this->boundary->width() / 2, this->boundary->height() / 2);
	}

	if(!this->ne) {
		this->ne = new QuadTree(this->boundary->center().x, this->boundary->center().y - this->boundary->height() * 0.5, this->boundary->width() / 2, this->boundary->height() / 2);
	}

	if(!this->sw) {
		this->sw = new QuadTree(this->boundary->center().x - this->boundary->width() * 0.5, this->boundary->center().y, this->boundary->width() / 2, this->boundary->height() / 2);
	}

	if(!this->se) {
		this->se = new QuadTree(this->boundary->center().x, this->boundary->center().y, this->boundary->width() / 2, this->boundary->height() / 2);
	}
}

int QuadTree::size() {
	int result = 1;
	if(this->ne){
		result += this->ne->size();
		result += this->nw->size();
		result += this->sw->size();
		result += this->se->size();
	}
	return result;
}

int QuadTree::point_count() {
	int result = this->points.size();
	if(this->ne){
		result += this->ne->point_count();
		result += this->nw->point_count();
		result += this->sw->point_count();
		result += this->se->point_count();
	}
	return result;
}

QuadTree::~QuadTree(void)
{
	//if(nw){
	//	delete nw;
	//}

	//if(ne){
	//	delete ne;
	//}

	//if(sw) {
	//	delete sw;
	//}

	//if(se){
	//	delete sw;
	//}

	//delete boundary;
}
