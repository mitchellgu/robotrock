#include <cassert>
#include <fstream>
#include <math.h>
#include <utility>
#include <vector>
#include <iostream>
#include <time.h>
#include <string>
#include "opencv2/opencv.hpp"

#define REP(i,n) for(int i=0;i<n;i++)
#define mp std::make_pair
#define pb push_back

using namespace cv;

const double distA=6.8327;
const double distB=0.0062;
const int distR=240;
const int distC=320;
typedef std::pair<double,double> pdd;
typedef std::pair<int,int> pii;

pdd getDist(int gi, int gj)
{
	std::ifstream file("out.txt");
	int x,y; double i,j;
	double minMatch=1e9;
	double oi,oj;
	while(!file.eof())
	{
		file>>x>>y;
		file>>i>>j;
		//std::cout<<x<<" "<<y<<" "<<i<<" "<<j<<std::endl;
		double dist=(x-gi)*(x-gi)+(y-gj)*(y-gj);
		if(dist<minMatch)
		{
			minMatch=dist;
			oi=i,oj=j;
		}
	}
	file.close();
	double angle=std::atan2(oi,oj)*180/3.14159;
	return mp(std::sqrt(oi*oi+oj*oj),angle);
}
pdd getOldDist(int i, int j)
{
	i=distR-i;
	j=j-(distC/2);
	double dist=distA*std::exp(distB*i);
	double angle=std::atan2(j,i);
	dist=dist/std::cos(angle);
	angle=angle*180/3.14159;
	return mp(dist,angle);
}
void downSize(Mat& inFrame, Mat& outFrame) 
{
	resize(inFrame, outFrame, Size(), 0.5, 0.5, INTER_NEAREST);
}
void hsv(Mat& inFrame, Mat& outFrame) 
{
	cvtColor(inFrame, outFrame, CV_BGR2HSV);
}
void maxFilter(Mat& frame, std::vector<int> inds)
{
	int* mxblue=new int[frame.cols];
	REP(i,frame.rows)
	{
		REP(j,frame.cols)
		{
			bool set=false;
			Vec3b& cur =frame.at<Vec3b>(i,j);
			for(int j=0;j<inds.size()&&!set;j++)
			{
				int ind=inds[j];
				double multA,multB;
				bool Y=false;
				if(ind==0) multA=1.22,multB=1.5;
				if(ind==1) multA=multB=1.2;
				if(ind==2) multA=multB=1.2;
				if(ind==3) Y=true,ind=1,multA=0.8,1.12;
				if(cur[ind]>75&&cur[ind]>multA*cur[(ind+1)%3]&&cur[ind]>multB*cur[(ind+2)%3])  
				{
					set=true;
					cur[ind]=255;
					cur[(ind+1)%3]=Y?255:0;
					cur[(ind+2)%3]=0;
				}
			}
			if(!set)
			{
				cur[0]=cur[1]=cur[2]=0;
			}
		}
	}
}
void maxFilter(Mat& frame, int ind, double multA=1.22, double multB=1.5)
{
	bool Y=false;
	if(ind!=0) multA=multB=1.2;
	if(ind==3) Y=true,ind=1,multA=0.8,1.12;
	REP(i,frame.rows)
	{
		REP(j,frame.cols)
		{
			Vec3b& cur =frame.at<Vec3b>(i,j);
			if(cur[ind]>60&&cur[ind]>multA*cur[(ind+1)%3]&&cur[ind]>multB*cur[(ind+2)%3])  
			{
				cur[ind]=255;
				cur[(ind+1)%3]=Y?255:0;
				cur[(ind+2)%3]=0;
			}
			else 
			{
				cur[ind]=cur[(ind+1)%3]=cur[(ind+2)%3]=0;
			}
		}
	}
}
int** comp; std::vector<std::pair<double,double> > cents; int currentComp;
int dimR, dimC, toti, totj;
int dx[]={1,-1,0,0};
int dy[]={0,0,1,-1};
bool check(int i, int j, Mat &inFrame)
{
	if(i<0||i>=dimR) return false;
	if(j<0||j>=dimC) return false;
	Vec3b &cur=inFrame.at<Vec3b>(i,j);
	if((cur[0]+cur[1]+cur[2])==0) return false;
	if(comp[i][j]==-1) return true;	
	return false;
}
bool checkin(int i, int j)
{
	if(i<0||i>=dimR) return false;
	if(j<0||j>=dimC) return false;
	return true;
}
int dfs(int i, int j, Mat &inFrame )
{
	int ar=1;
	toti+=i, totj+=j;
	comp[i][j]=currentComp;
	REP(k,4)
	{
		int ni=i+dx[k],nj=j+dy[k];
		if(!check(ni,nj,inFrame)) continue;
		ar+=dfs(ni,nj,inFrame);
	}
	return ar;
}
struct centers 
{
	double dist;
	double angle;
	int type;
};
std::vector<centers> fill(Mat &inFrame)
{
	comp=new int*[inFrame.rows];
	currentComp=0;
	dimR=inFrame.rows, dimC=inFrame.cols;
	//std::cout<<dimR<<" "<<dimC<<std::endl;
	REP(i,inFrame.rows)
	{
		comp[i]=new int[inFrame.cols];
		REP(j,inFrame.cols) comp[i][j]=-1;
	}
	std::vector<centers> out;
	REP(i,inFrame.rows) REP(j,inFrame.cols) 
	{
		if(!check(i,j,inFrame)) continue;
		toti=0,totj=0;
		int ar=dfs(i,j,inFrame);
		int ci=toti/((double)ar);
		int cj=totj/((double)ar);
		if(ar>=500) 
		{
			//std::cout<<"("<<ci<<", "<<cj<<"), area: "<<ar<<std::endl;
			cents.pb(mp(ci,cj));
			std::pair<double,double> dist=getDist(ci,cj);
			//std::cout<<"This point is "<<dist.first<<" inches away at angle "<<dist.second<<" to the normal \n";
			centers add;
			add.dist=dist.first;
			add.angle=dist.second;
			Vec3b &cur=inFrame.at<Vec3b>(i,j);
			if(cur[0]!=0) add.type=0;
			else if(cur[1]!=0&&cur[2]!=0) add.type=3;
			else if(cur[1]!=0) add.type=1;
			else add.type=2;
			REP(x,5) REP(y,5) 
			{
				int ni=ci+x,nj=cj+y;
				if(checkin(ni,nj))
				{
					//std::cout<<ni<<"" <<nj<<std::endl;
					int ind=1;
					inFrame.at<Vec3b>(ni,nj)[ind]=0,inFrame.at<Vec3b>(ni,nj)[(ind+2)%3]=255;
				}
			}
			out.push_back(add);
		}
		currentComp++;
	}
	return out;
}
pdd fill(Mat &inFrame,int ind)
{
	if(ind==3) ind=1;
	comp=new int*[inFrame.rows];
	cents.resize(0);
	currentComp=0;
	dimR=inFrame.rows, dimC=inFrame.cols;
	//std::cout<<dimR<<" "<<dimC<<std::endl;
	REP(i,inFrame.rows)
	{
		comp[i]=new int[inFrame.cols];
		REP(j,inFrame.cols) comp[i][j]=-1;
	}
	pdd ret=mp(1e9,0);
	REP(i,inFrame.rows) REP(j,inFrame.cols) 
	{
		if(!check(i,j,inFrame)) continue;
		toti=0,totj=0;
		int ar=dfs(i,j,inFrame);
		int ci=toti/((double)ar);
		int cj=totj/((double)ar);
		if(ar>=200) 
		{
			//std::cout<<"("<<ci<<", "<<cj<<"), area: "<<ar<<std::endl;
			cents.pb(mp(ci,cj));
			std::pair<double,double> dist=getDist(ci,cj);
			//std::cout<<"This point is "<<dist.first<<" inches away at angle "<<dist.second<<" to the normal \n";
			ret=dist;
			REP(x,5) REP(y,5) 
			{
				int ni=ci+x,nj=cj+y;
				if(checkin(ni,nj))
				{
					//std::cout<<ni<<"" <<nj<<std::endl;
					inFrame.at<Vec3b>(ni,nj)[ind]=0,inFrame.at<Vec3b>(ni,nj)[(ind+2)%3]=255;
				}
			}
		}
		currentComp++;
	}
	return ret;
}
Mat brigChange(Mat frame)
{
	double alpha=2.0;
	int beta=80;
	Mat new_image=Mat::zeros( frame.size(), frame.type() );
	for( int y = 0; y < frame.rows; y++ )
	{ 
		for( int x = 0; x < frame.cols; x++ )
		{ 
			for( int c = 0; c < 3; c++ ) new_image.at<Vec3b>(y,x)[c] = saturate_cast<uchar>( alpha*( frame.at<Vec3b>(y,x)[c] ) + beta );
		}
	}
	return new_image;
}
void edgeDetect(Mat& inFrame, Mat& outFrame)
{
	Mat kern = (Mat_<char>(3,3) <<  0, -1,  0,
			-1,  4, -1,
			0, -1,  0);
	filter2D(inFrame, outFrame, inFrame.depth(), kern);
}
Mat canny(Mat &inFrame)
{
	const int edgeThresh=20;
	Mat gray,edge,blu;
	cvtColor(inFrame, gray, COLOR_BGR2GRAY);
	blur(gray, blu, Size(3,3));
	Canny(blu, edge, edgeThresh, edgeThresh*3, 3);
	//out.create(inFrame.size(),inFrame.type());
	//out= Scalar::all(0);
	//inFrame.copyTo(out,edge);
	return edge;
}
