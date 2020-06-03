#pragma once

#include <vector>
#include <stdint.h>

using namespace std;

#define	PI				3.1415926535897932384626433832795

typedef struct _VECTOR3D_
{
	_VECTOR3D_() {
	
	}

	_VECTOR3D_(double x, double y, double z) {
		val[0] = x; val[1] = y; val[2] = z;
	}
	double	val[3];
}VECTOR3D, *PVECTOR3D;

typedef struct _VERTEX_
{
	_VERTEX_() {
		val[0] = 0; val[1] = 0; val[2] = 0; val[3] = 1;
		ntriCnt = 0;
		optIndex = -1;
	}

	_VERTEX_(double x, double y, double z) {
		val[0] = x; val[1] = y; val[2] = z; val[3] = 1;
		ntriCnt = 0;
		optIndex = -1;
	}
	double		val[4];
	VECTOR3D	nor;
	int			ntriCnt;
	int			optIndex;
}VERTEX, *PVERTEX;

typedef struct _TRIANGLE_
{
	_TRIANGLE_() {
		idx[0] = -1; idx[1] = -1; idx[2] = -1;
	}

	_TRIANGLE_(int idx1, int idx2, int idx3) {
		idx[0] = idx1; idx[1] = idx2; idx[2] = idx3;
	}
	int		idx[3];
	double	param[4];
}TRIANGLE, *PTRIANGLE;

typedef struct _SUBNODE_
{
	vector<VERTEX>		vertex;
	vector<TRIANGLE>	index;
}SUBNODE, *PSUBNODE;

typedef struct _MATRIX_ 
{
	double val[4][4];
}MATRIX, *PMATRIX;

enum FBXENUM
{
	SUCCESS_IMPORT,
	INVALID_FBX_FILE,
	UNSURPPORTED_FBX_VERSION,
	UNSURPPORTED_PROPERTY_TYPE,
	INVALID_VERTEX_PAIR,
	INVALID_POLYGON_PAIR,
	COMPRESSED_PROPERTY_FOUND
};

extern int RESIZED_WIDTH;
extern int RESIZED_HEIGHT;
extern int ROI;

extern vector<SUBNODE>	nodes;

extern double	*depth;
extern int		*indexMap1;
extern int		*indexMap2;
extern VERTEX	*vertMap;
extern unsigned char	*renderImg;

extern double	roi_Depth;
extern int		gridInv;
extern double	maxDepth;
extern double	minDepth;
extern double	x_Rotate;
extern double	y_Rotate;
extern double	z_Rotate;
extern double	zoomFactor;
extern double	normalFactor;

extern vector<SUBNODE>	nodes;
extern vector<SUBNODE>	tNodes;
extern vector<SUBNODE>	eNodes;

extern VECTOR3D tDirV;

FBXENUM readFBX(char *fileName);
int readOBJ(char *fileName);
int readSTL(char *fileName);
bool isLittleEndian();
char readChar(FILE *pFile);
std::string readString(FILE *pFile, uint8_t length);
uint32_t readUint32(FILE *pFILE);
uint64_t readUint64(FILE *pFILE);
uint16_t readUint16(FILE *pFile);
float readFloat(FILE *pFile);
double readDouble(FILE *pFile);
uint32_t arrayElementSize(char type);
void log(FBXENUM msg);
bool FileExists(const char *filePathPtr);
std::string toString(float v);
void init3DOpt();
void OptimizeMesh(bool optFlag);

//3D Opt
void getCoordinate();
void CorrectView();
void doTransform();
void doFindEuqation();
void doRender();
void normalizeDepth();
void doProjection(VERTEX *a);
VERTEX doCross(VERTEX *a, VERTEX *b);
VECTOR3D doCross(VECTOR3D *a, VECTOR3D *b);
void doNormalize(VERTEX *v);
void doNormalize(VECTOR3D *v);
double doMagnitude(VECTOR3D *v);
double doMagnitudeForVertex(VERTEX *v);
void doProjection(VECTOR3D *a);
VERTEX doMinus(VERTEX *a, VERTEX *b);
VECTOR3D doMinus(VECTOR3D *a, VECTOR3D *b);
void PutColor(double x, double y, int r, int g, int b);
void DrawLine(double x1, double y1, double x2, double y2, int r, int g, int b, int width);
void DrawRotationMark();
void CopyMesh();
void DoMakeBolder();
void DoMakeThiner();

void InitRenderBuffer(int width, int height);
void ReleaseRenderBuffer();