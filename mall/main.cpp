#include <stdlib.h>
#include <GL/glut.h>
#include "imageloader.h"
#include <fstream>
#include <assert.h>
#include <math.h>
using namespace std;

float angle = 0;
float xScale=1,yScale=1,zangle=0;
float mainDoorLeft = 0, mainDoorRight = 0;
float exitDoorLeft = 0, exitDoorRight = 0;
int is_depth;
GLUquadricObj *Disk;

// actual vector representing the camera's direction
float lx=0.0f,lz=-1.0f;

// XZ position of the camera
float x=0.0f, z=5.0f;

// the key states. These variables will be zero
//when no key is being presses
float deltaAngle = 0.0f;
float deltaMove = 0;
int xOrigin = -1;

//var texture
GLuint
    _textureBrick,
    _textureDoor,
    _textureGrass,
    _textureRoof,
    _textureWindow,
    _textureSky,
    _textureStreet,
    _textureOuterLayer,
    _textureWallInside,
    _textureWindow1,
    _textureGrey,
    _textureLogo1,
    _texturedoor2,
    _textureLogo2,
    _textureWindow2,
    _textureMallFloor,
    _textureStairWall,
    _textureDoorL,
    _textureDoorR,
    _textureLogo3,
    _textureRed;

//imageloader
Image::Image(char* ps, int w, int h) : pixels(ps), width(w), height(h) {

}

Image::~Image() {
	delete[] pixels;
}

namespace {
	//Converts a four-character array to an integer, using little-endian form
	int toInt(const char* bytes) {
		return (int)(((unsigned char)bytes[3] << 24) |
					 ((unsigned char)bytes[2] << 16) |
					 ((unsigned char)bytes[1] << 8) |
					 (unsigned char)bytes[0]);
	}

	//Converts a two-character array to a short, using little-endian form
	short toShort(const char* bytes) {
		return (short)(((unsigned char)bytes[1] << 8) |
					   (unsigned char)bytes[0]);
	}

	//Reads the next four bytes as an integer, using little-endian form
	int readInt(ifstream &input) {
		char buffer[4];
		input.read(buffer, 4);
		return toInt(buffer);
	}

	//Reads the next two bytes as a short, using little-endian form
	short readShort(ifstream &input) {
		char buffer[2];
		input.read(buffer, 2);
		return toShort(buffer);
	}

	//Just like auto_ptr, but for arrays
	template<class T>
	class auto_array {
		private:
			T* array;
			mutable bool isReleased;
		public:
			explicit auto_array(T* array_ = NULL) :
				array(array_), isReleased(false) {
			}

			auto_array(const auto_array<T> &aarray) {
				array = aarray.array;
				isReleased = aarray.isReleased;
				aarray.isReleased = true;
			}

			~auto_array() {
				if (!isReleased && array != NULL) {
					delete[] array;
				}
			}

			T* get() const {
				return array;
			}

			T &operator*() const {
				return *array;
			}

			void operator=(const auto_array<T> &aarray) {
				if (!isReleased && array != NULL) {
					delete[] array;
				}
				array = aarray.array;
				isReleased = aarray.isReleased;
				aarray.isReleased = true;
			}

			T* operator->() const {
				return array;
			}

			T* release() {
				isReleased = true;
				return array;
			}

			void reset(T* array_ = NULL) {
				if (!isReleased && array != NULL) {
					delete[] array;
				}
				array = array_;
			}

			T* operator+(int i) {
				return array + i;
			}

			T &operator[](int i) {
				return array[i];
			}
	};
}

Image* loadBMP(const char* filename) {
	ifstream input;
	input.open(filename, ifstream::binary);
	assert(!input.fail() || !"Could not find file");
	char buffer[2];
	input.read(buffer, 2);
	assert(buffer[0] == 'B' && buffer[1] == 'M' || !"Not a bitmap file");
	input.ignore(8);
	int dataOffset = readInt(input);

	//Read the header
	int headerSize = readInt(input);
	int width;
	int height;
	switch(headerSize) {
		case 40:
			//V3
			width = readInt(input);
			height = readInt(input);
			input.ignore(2);
			assert(readShort(input) == 24 || !"Image is not 24 bits per pixel");
			assert(readShort(input) == 0 || !"Image is compressed");
			break;
		case 12:
			//OS/2 V1
			width = readShort(input);
			height = readShort(input);
			input.ignore(2);
			assert(readShort(input) == 24 || !"Image is not 24 bits per pixel");
			break;
		case 64:
			//OS/2 V2
			assert(!"Can't load OS/2 V2 bitmaps");
			break;
		case 108:
			//Windows V4
			assert(!"Can't load Windows V4 bitmaps");
			break;
		case 124:
			//Windows V5
			assert(!"Can't load Windows V5 bitmaps");
			break;
		default:
			assert(!"Unknown bitmap format");
	}

	//Read the data
	int bytesPerRow = ((width * 3 + 3) / 4) * 4 - (width * 3 % 4);
	int size = bytesPerRow * height;
	auto_array<char> pixels(new char[size]);
	input.seekg(dataOffset, ios_base::beg);
	input.read(pixels.get(), size);

	//Get the data into the right format
	auto_array<char> pixels2(new char[width * height * 3]);
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			for(int c = 0; c < 3; c++) {
				pixels2[3 * (width * y + x) + c] =
					pixels[bytesPerRow * y + 3 * x + (2 - c)];
			}
		}
	}

	input.close();
	return new Image(pixels2.release(), width, height);
}
//imageloader

static void resize(int width, int height)
{
    const float ar = (float) width / (float) height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glEnable(GL_DEPTH_TEST);
    is_depth=1;
    glLoadIdentity();
    glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);
    glMatrixMode(GL_MODELVIEW);

    //create cube
    Disk = gluNewQuadric();
    gluQuadricDrawStyle( Disk, GLU_FILL);
    gluQuadricNormals( Disk, GLU_SMOOTH);

    glLoadIdentity();
}

void setupLights()
{
	GLfloat ambient[] = {0.7f, 0.7f, 0.7, 1.0f}; //cahaya tdk dapat ditebtukn (segala arah)
	GLfloat diffuse[] = {0.6f, 0.6f, 0.6, 1.0f}; //cahaya bersumber sari 1 arah
	GLfloat specular[] = {0.4f, 0.4f, 0.4, 0.4f}; // cahaya yang memantul
	GLfloat shininess[] = {50}; //kesan mengkilap
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

	GLfloat lightIntensity[] = {0.7f, 0.7f, 1, 1.0f};
	glLightfv(GL_LIGHT0, GL_POSITION, lightIntensity);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightIntensity);
}

void drawSky()
{
    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, _textureSky);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-35);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,1.0,0.1);  glVertex3f(-60,60,0);
            glTexCoord3f(1.0,1.0,0.1);  glVertex3f(60,60,0);
            glTexCoord3f(1.0,0.0,0.1);  glVertex3f(60,-60,0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-60,-60,0);
        glEnd();
    glPopMatrix();
}

void drawStreet()
{
    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, _textureStreet);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0, 1.0, 0.0);
        glBegin(GL_QUADS);
            glTexCoord3f(70.0,0.0,1);  glVertex3f(-35,-1.5,35);
            glTexCoord3f(0.0,0.0,-1);  glVertex3f(-35,-1.5,-35);
            glTexCoord3f(0.0,70.0,-1);  glVertex3f(55,-1.5,-35);
            glTexCoord3f(70.0,70.0,1);  glVertex3f(55,-1.5,35);
        glEnd();
    glPopMatrix();
}

void drawFloor()
{
    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, _textureMallFloor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0, 1.0, 0.0);
        glTranslatef(4.0,0.0001,-6.0);
        glScalef(1.0,1.0,1.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(15.0,0.0,1);  glVertex3f(-6,-1.5,7);
            glTexCoord3f(0.0,0.0,-1);  glVertex3f(-6,-1.5,-6);
            glTexCoord3f(0.0,15.0,-1);  glVertex3f(21.7,-1.5,-6);
            glTexCoord3f(15.0,15.0,1);  glVertex3f(21.7,-1.5,7);
        glEnd();
    glPopMatrix();

    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, _textureMallFloor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0, 1.0, 0.0);
        glTranslatef(4.0,7.0,-6.0);
        glScalef(1.0,1.0,1.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(15.0,0.0,1);  glVertex3f(-6,-1.5,7);
            glTexCoord3f(0.0,0.0,-1);  glVertex3f(-6,-1.5,-6);
            glTexCoord3f(0.0,15.0,-1);  glVertex3f(21.7,-1.5,-6);
            glTexCoord3f(15.0,15.0,1);  glVertex3f(21.7,-1.5,7);
        glEnd();
    glPopMatrix();
}

void drawBase_Front()
{
    //wall_a front wall
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,-1.9,0.1);  glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(1.0,-1.9,0.1);  glVertex3f(-1.0,1.0,1.0);
        glTexCoord3f(1.0,0.0,0.1);  glVertex3f(-1.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();
    glPopMatrix();

    //wall_a right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(0.0,1.0,0.0);
        glScalef(1.0,1.9,1.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-2.5,0.1);   glVertex3f(-1.0,0.0,-12.0);
            glTexCoord3f(8.5,-2.5,0.1);   glVertex3f(-1.0,0.0,1.0);
            glTexCoord3f(8.5,0.0,0.1);  glVertex3f(-1.0,-1.5,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-1.0,-1.5,-12.0);
        glEnd();
        glPopMatrix();

    //wall_b left
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-1.3,0.1);     glVertex3f(1.5,0.0,-1.0);
            glTexCoord3f(2.5,-1.3,0.1);     glVertex3f(1.5,0.0,1.0);
            glTexCoord3f(2.5,0.0,0.1);      glVertex3f(1.5,-1.5,1.0);
            glTexCoord3f(0.0,0.0,0.1);      glVertex3f(1.5,-1.5,-1.0);
        glEnd();
        glPopMatrix();

    //wall_b right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-1.3,0.1);     glVertex3f(2.2,0.0,-1.0);
            glTexCoord3f(2.5,-1.3,0.1);     glVertex3f(2.2,0.0,1.0);
            glTexCoord3f(2.5,0.0,0.1);      glVertex3f(2.2,-1.5,1.0);
            glTexCoord3f(0.0,0.0,0.1);      glVertex3f(2.2,-1.5,-1.0);
        glEnd();
        glPopMatrix();

    //wall_b back
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-1.3,0.1);     glVertex3f(1.5,0.0,-1.0);
            glTexCoord3f(2.5,-1.3,0.1);     glVertex3f(2.2,0.0,-1.0);
            glTexCoord3f(2.5,0.0,0.1);      glVertex3f(2.2,-1.5,-1.0);
            glTexCoord3f(0.0,0.0,0.1);      glVertex3f(1.5,-1.5,-1.0);
        glEnd();
        glPopMatrix();

    //wall_b front
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-1.3,0.1);     glVertex3f(1.5,0.0,1.0);
            glTexCoord3f(2.5,-1.3,0.1);     glVertex3f(2.2,0.0,1.0);
            glTexCoord3f(2.5,0.0,0.1);      glVertex3f(2.2,-1.5,1.0);
            glTexCoord3f(0.0,0.0,0.1);      glVertex3f(1.5,-1.5,1.0);
        glEnd();
        glPopMatrix();

    //wall_c front entrance
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(2.0,0.08,0.2);
        glScaled(6.0, 0.17, 2.0);
        glColor3f(0.95, 0.95, 0.95);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

    //wall_d upper front wall
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(3.5,0.0,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,1.0,0.1);  glVertex3f(-4.5,1.0,1.0);
        glTexCoord3f(5.0,1.0,0.1);  glVertex3f(1.5,1.0,1.0);
        glTexCoord3f(5.0,0.0,0.1);  glVertex3f(1.5,-0.0,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-4.5,-0.0,1.0);
    glEnd();
    glPopMatrix();

    //wall_d left
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-1.3,0.1);   glVertex3f(5.0,0.0,-1.0);
            glTexCoord3f(2.5,-1.3,0.1);   glVertex3f(5.0,0.0,1.0);
            glTexCoord3f(2.5,0.0,0.1);  glVertex3f(5.0,-1.5,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(5.0,-1.5,-1.0);
        glEnd();
        glPopMatrix();

	//wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(7.0,0.0,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(4.0,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(4.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();

    //wall_dd windows right
    glTranslatef(-0.75,0.0,0.0);
    glBegin(GL_QUADS);
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(1.5,-0.8,1.0001);
        glEnd();
    glPopMatrix();
    //end

    //wall_e front wall MG entrance
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(13,0.0,0.1);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,-2.9,0.1);  glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(1.0,-2.9,0.1);  glVertex3f(-1.0,1.0,1.0);
        glTexCoord3f(1.0,0.0,0.1);  glVertex3f(-1.0,-2.5,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-2.5,1.0);
    glEnd();
    glPopMatrix();

    //wall_e left
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(6.0,1.0,1.0);
        glScalef(xScale,2.0,0.1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-1.3,0.1);   glVertex3f(5.0,0.0,-1.0);
            glTexCoord3f(2.5,-1.3,0.1);   glVertex3f(5.0,0.0,1.0);
            glTexCoord3f(2.5,0.0,0.1);  glVertex3f(5.0,-1.5,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(5.0,-1.5,-1.0);
        glEnd();

        //wall_e right
        glTranslatef(1.0,0.0,0.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-1.3,0.1);   glVertex3f(5.0,0.0,-1.0);
            glTexCoord3f(2.5,-1.3,0.1);   glVertex3f(5.0,0.0,1.0);
            glTexCoord3f(2.5,0.0,0.1);  glVertex3f(5.0,-1.5,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(5.0,-1.5,-1.0);
        glEnd();

        //wall_e upper
        glTranslatef(7.0,-0.08,0.0);
        glScalef(1.0,0.080,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-2.9,0.1);  glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(1.0,-2.9,0.1);  glVertex3f(-1.0,1.0,1.0);
        glTexCoord3f(1.0,0.0,0.1);  glVertex3f(-1.0,-2.5,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-2.5,1.0);
        glEnd();
        glPopMatrix();


    //wall_f front wall glass
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureGrey);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(14,0.0,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,-2.9,0.1);  glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(1.0,-2.9,0.1);  glVertex3f(-1.0,1.0,1.0);
        glTexCoord3f(1.0,0.0,0.1);  glVertex3f(-1.0,-2.5,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-2.5,1.0);
    glEnd();

    //wall_f windows
    glBindTexture(GL_TEXTURE_2D, _textureLogo1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0.1,1.20,0.0);
        glScalef(1.4,2.3,1);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();
    glPopMatrix();

    //wall_f leftside
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(8.0,1.0,1.0);
        glScalef(xScale,2.0,0.1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-1.3,0.1);   glVertex3f(5.0,0.0,-1.0);
            glTexCoord3f(2.5,-1.3,0.1);   glVertex3f(5.0,0.0,1.0);
            glTexCoord3f(2.5,0.0,0.1);  glVertex3f(5.0,-1.5,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(5.0,-1.5,-1.0);
        glEnd();
        glPopMatrix();

    //wall_g front wall
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(12.40,0.0,0.1);
    glScalef(-0.60,1.0,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,-2.9,0.1);  glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(1.0,-2.9,0.1);  glVertex3f(-1.0,1.0,1.0);
        glTexCoord3f(1.0,0.0,0.1);  glVertex3f(-1.0,-2.5,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-2.5,1.0);
    glEnd();
    glPopMatrix();

    //wall_h front wall
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(20.60,0.0,0.1);
    glScalef(3.5,1.0,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,-2.0,0.1);  glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(3.0,-2.0,0.1);  glVertex3f(-1.0,1.0,1.0);
        glTexCoord3f(3.0,0.0,0.1);  glVertex3f(-1.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_h door
    glBindTexture(GL_TEXTURE_2D, _texturedoor2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(-0.94,0.80,0.0);
    glScalef(0.5,2.9,1);
    glBegin(GL_QUADS);  // Window Left
        glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
        glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
        glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
        glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();

        //wall_h banner
    glBindTexture(GL_TEXTURE_2D, _textureLogo2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0.75,0.069,0.0);
    glScalef(1.15,0.80,1);
    glBegin(GL_QUADS);
        glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.5,0.0,1.0001);
        glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,0.0,1.0001);
        glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.2,1.0001);
        glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.5,-0.2,1.0001);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, _textureLogo3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(2.0,2.73,-2.1);
    glScalef(3.0,3.0,3.0);
    glBegin(GL_QUADS);
        glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.5,0.0,1.0001);
        glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,0.0,1.0001);
        glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.2,1.0001);
        glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.5,-0.2,1.0001);
        glEnd();
    glPopMatrix();

    //wall_i front wall MG_Right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(16.50,0.0,0.1);
    glScalef(-0.60,1.0,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,-2.9,0.1);  glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(1.0,-2.9,0.1);  glVertex3f(-1.0,1.0,1.0);
        glTexCoord3f(1.0,0.0,0.1);  glVertex3f(-1.0,-2.5,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-2.5,1.0);
    glEnd();
    glPopMatrix();

    //wall_i front wall glass
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureGrey);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(19.70,0.0,0.1);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,-2.9,0.1);  glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(1.0,-2.9,0.1);  glVertex3f(-1.0,1.0,1.0);
        glTexCoord3f(1.0,0.0,0.1);  glVertex3f(-1.0,-2.5,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-2.5,1.0);
    glEnd();

    //wall_i windows
    glBindTexture(GL_TEXTURE_2D, _textureLogo1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0.1,1.20,0.0);
        glScalef(1.4,2.3,1);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();
    glPopMatrix();

    //wall_i upper
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        //wall_e upper
        glTranslatef(19.70,0.86,0.1001);
        glScalef(1.0,0.14,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-2.9,0.1);  glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(1.0,-2.9,0.1);  glVertex3f(-1.0,1.0,1.0);
        glTexCoord3f(1.0,0.0,0.1);  glVertex3f(-1.0,-2.5,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-2.5,1.0);
        glEnd();
        glPopMatrix();

    //wall_i front wall
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(20.70,0.0,0.101);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,-2.9,0.1);  glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(1.0,-2.9,0.1);  glVertex3f(-1.0,1.0,1.0);
        glTexCoord3f(1.0,0.0,0.1);  glVertex3f(-1.0,-2.5,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-2.5,1.0);
    glEnd();
    glPopMatrix();

    //wall_j front corner wall right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(21.70,0.0,0.1);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(4.0,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(4.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_j windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();

    //wall_j windows right
    glTranslatef(-0.75,0.0,0.0);
    glBegin(GL_QUADS);
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(1.5,-0.8,1.0001);
        glEnd();
    glPopMatrix();

}

void drawBase_Right()
{
    //wall_a right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(26.70,1.0,0.1);
        glScalef(1.0,1.9,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-2.0,0.1);   glVertex3f(-1.0,0.0,-12.1);
            glTexCoord3f(10.5,-2.0,0.1);   glVertex3f(-1.0,0.0,1.0);
            glTexCoord3f(10.5,0.0,0.1);  glVertex3f(-1.0,-1.35,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-1.0,-1.35,-12.1);
        glEnd();

        //wall_j windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.0001,-0.15,0.94);
        glScalef(1.0,1.3,1.6);
        glBegin(GL_QUADS);  // Window 1
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

    //wall_j windows right
    glTranslatef(0.0,0.0,-1.55);
    glBegin(GL_QUADS);  // Window 2
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

    //wall_j windows right
    glTranslatef(0.0,0.0,-2.32);
    glBegin(GL_QUADS);  // Window 3
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

    //wall_j windows right
    glTranslatef(0.0,0.0,-1.545);
    glBegin(GL_QUADS);  // Window 4
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

        glPopMatrix();

        ///////////

        //wall_a right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(26.70,3.50,0.1);
        glScalef(1.0,1.9,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-2.0,0.1);   glVertex3f(-1.0,0.0,-12.1);
            glTexCoord3f(10.5,-2.0,0.1);   glVertex3f(-1.0,0.0,1.0);
            glTexCoord3f(10.5,0.0,0.1);  glVertex3f(-1.0,-1.35,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-1.0,-1.35,-12.1);
        glEnd();

        //wall_j windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.0001,-0.15,0.94);
        glScalef(1.0,1.3,1.6);
        glBegin(GL_QUADS);  // Window 1
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

    //wall_j windows right
    glTranslatef(0.0,0.0,-1.55);
    glBegin(GL_QUADS);  // Window 2
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

    //wall_j windows right
    glTranslatef(0.0,0.0,-2.32);
    glBegin(GL_QUADS);  // Window 3
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

    //wall_j windows right
    glTranslatef(0.0,0.0,-1.545);
    glBegin(GL_QUADS);  // Window 4
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

        glPopMatrix();

        ///////////

        //wall_a right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(26.70,6.0,0.1);
        glScalef(1.0,1.9,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-2.0,0.1);   glVertex3f(-1.0,0.0,-12.1);
            glTexCoord3f(10.5,-2.0,0.1);   glVertex3f(-1.0,0.0,1.0);
            glTexCoord3f(10.5,0.0,0.1);  glVertex3f(-1.0,-1.35,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-1.0,-1.35,-12.1);
        glEnd();

        //wall_j windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.0001,-0.15,0.94);
        glScalef(1.0,1.3,1.6);
        glBegin(GL_QUADS);  // Window 1
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

    //wall_j windows right
    glTranslatef(0.0,0.0,-1.55);
    glBegin(GL_QUADS);  // Window 2
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

    //wall_j windows right
    glTranslatef(0.0,0.0,-2.32);
    glBegin(GL_QUADS);  // Window 3
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

    //wall_j windows right
    glTranslatef(0.0,0.0,-1.545);
    glBegin(GL_QUADS);  // Window 4
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-1.5);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-2.0,-0.3,-0.75);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-0.75);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-2.0,-0.8,-1.5);
        glEnd();

        glPopMatrix();
}

void drawBase_Left()
{
    //wall_a right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(-1.0,1.0,0.0);
        glScalef(1.0,1.9,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-2.0,0.1);   glVertex3f(-1.0,0.0,-12.0);
            glTexCoord3f(10.5,-2.0,0.1);   glVertex3f(-1.0,0.0,1.0);
            glTexCoord3f(10.5,0.0,0.1);  glVertex3f(-1.0,-1.35,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-1.0,-1.35,-12.0);
        glEnd();

        glPopMatrix();

        //wall_a right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(-1.0,3.50,0.0);
        glScalef(1.0,1.9,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-2.0,0.1);   glVertex3f(-1.0,0.0,-12.0);
            glTexCoord3f(10.5,-2.0,0.1);   glVertex3f(-1.0,0.0,1.0);
            glTexCoord3f(10.5,0.0,0.1);  glVertex3f(-1.0,-1.35,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-1.0,-1.35,-12.0);
        glEnd();

        glPopMatrix();

        //wall_a right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(-1.0,6.0,0.0);
        glScalef(1.0,1.9,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-2.0,0.1);   glVertex3f(-1.0,0.0,-12.0);
            glTexCoord3f(10.5,-2.0,0.1);   glVertex3f(-1.0,0.0,1.0);
            glTexCoord3f(10.5,0.0,0.1);  glVertex3f(-1.0,-1.35,1.0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-1.0,-1.35,-12.0);
        glEnd();

        glPopMatrix();
}

void drawBase_Behind()
{
    //wall_b back
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(-3.50,0.0,-11);
        glScalef(1.0,1.0,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-1.95,0.1);     glVertex3f(1.5,1.0,-1.0);
            glTexCoord3f(20.5,-1.95,0.1);     glVertex3f(29.2,1.0,-1.0);
            glTexCoord3f(20.5,0.0,0.1);      glVertex3f(29.2,-1.5,-1.0);
            glTexCoord3f(0.0,0.0,0.1);      glVertex3f(1.5,-1.5,-1.0);
        glEnd();
        glPopMatrix();

        //wall_b back
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(-3.50,2.50,-11);
        glScalef(1.0,1.0,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-1.95,0.1);     glVertex3f(1.5,1.0,-1.0);
            glTexCoord3f(20.5,-1.95,0.1);     glVertex3f(29.2,1.0,-1.0);
            glTexCoord3f(20.5,0.0,0.1);      glVertex3f(29.2,-1.5,-1.0);
            glTexCoord3f(0.0,0.0,0.1);      glVertex3f(1.5,-1.5,-1.0);
        glEnd();
        glPopMatrix();

        //wall_b back
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(-3.50,5.0,-11);
        glScalef(1.0,1.0,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-1.95,0.1);     glVertex3f(1.5,1.0,-1.0);
            glTexCoord3f(20.5,-1.95,0.1);     glVertex3f(29.2,1.0,-1.0);
            glTexCoord3f(20.5,0.0,0.1);      glVertex3f(29.2,-1.5,-1.0);
            glTexCoord3f(0.0,0.0,0.1);      glVertex3f(1.5,-1.5,-1.0);
        glEnd();
        glPopMatrix();
}

void drawStairs()
{
    //stairs wall right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureStairWall);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(24.0,1.0,1.0);
        glScalef(1.0,1.9,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-0.50,0.1);   glVertex3f(-1.0,-0.75,-10.0);//left_atas
            glTexCoord3f(3.5,-0.50,0.1);   glVertex3f(-1.0,0.50,-4.0);//right_atas
            glTexCoord3f(3.5,0.0,0.1);  glVertex3f(-1.0,0.0,-4.0);//right_bawah
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-1.0,-1.35,-10.0);//left_bawah
        glEnd();
        glPopMatrix();

    //stairs floor
    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, _textureGrey);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0, 1.0, 0.0);
        glTranslatef(4.0,0.00012,-6.0);
        glScalef(1.0,1.0,1.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(15.0,0.0,1);  glVertex3f(17.50,1.0,3);//left_depan
            glTexCoord3f(0.0,0.0,-1);  glVertex3f(17.50,-1.5,-3.0);//left_belakang
            glTexCoord3f(0.0,15.0,-1);  glVertex3f(19.0,-1.5,-3.0);//right_belakang
            glTexCoord3f(15.0,15.0,1);  glVertex3f(19.0,1.0,3);//right_depan
        glEnd();
    glPopMatrix();

    //stairs wall left
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureStairWall);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(22.50,1.0,1.0);
        glScalef(1.0,1.9,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-0.50,0.1);   glVertex3f(-1.0,-0.75,-10.0);//left_atas
            glTexCoord3f(3.5,-0.50,0.1);   glVertex3f(-1.0,0.50,-4.0);//right_atas
            glTexCoord3f(3.5,0.0,0.1);  glVertex3f(-1.0,0.0,-4.0);//right_bawah
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-1.0,-1.35,-10.0);//left_bawah
        glEnd();
        glPopMatrix();

    //another floor
    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, _textureGrey);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0, 1.0, 0.0);
        glTranslatef(4.0,0.00013,-6.0);
        glScalef(1.0,1.0,1.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(15.0,0.0,1);  glVertex3f(19,1.0,3);//left_depan
            glTexCoord3f(0.0,0.0,-1);  glVertex3f(19,1.0,-6);//left_belakang
            glTexCoord3f(0.0,15.0,-1);  glVertex3f(21.7,1.0,-6);//right_belakang
            glTexCoord3f(15.0,15.0,1);  glVertex3f(21.7,1.0,3);//right_depan
        glEnd();
    glPopMatrix();

    //another floor wall 1
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureStairWall);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(24.0,1.0,1.0);
        glScalef(1.0,1.9,1.0);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,-0.70,0.1);   glVertex3f(-1.0,0.50,-13.0);//left_atas
            glTexCoord3f(3.5,-0.70,0.1);   glVertex3f(-1.0,0.50,-4.0);//right_atas
            glTexCoord3f(3.5,0.0,0.1);  glVertex3f(-1.0,0.0,-4.0);//right_bawah
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-1.0,0.0,-13.0);//left_bawah
        glEnd();
        glPopMatrix();

    //another floor 2
    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, _textureGrey);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0, 1.0, 0.0);
        glTranslatef(4.0,0.00013,-6.0);
        glScalef(1.0,1.0,1.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(15.0,0.0,1);  glVertex3f(-6,1.0,7.1);//left_depan
            glTexCoord3f(0.0,0.0,-1);  glVertex3f(-6,1.0,3);//left_belakang
            glTexCoord3f(0.0,15.0,-1);  glVertex3f(21.7,1.0,3);//right_belakang
            glTexCoord3f(15.0,15.0,1);  glVertex3f(21.7,1.0,7.1);//right_depan
        glEnd();
    glPopMatrix();

    //another floor wall 2
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureStairWall);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(0.0,1.0,0.0);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,-0.7,0.1);     glVertex3f(-2,0.0,-3.0);
            glTexCoord3f(5.8,-0.7,0.1);     glVertex3f(21.50,0.0,-3.0);
            glTexCoord3f(5.8,0.0,0.1);      glVertex3f(21.50,1.0,-3.0);
            glTexCoord3f(0.0,0.0,0.1);      glVertex3f(-2,1.0,-3.0);
        glEnd();
        glPopMatrix();

        //another floor 2
    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, _textureGrey);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0, 1.0, 0.0);
        glTranslatef(4.0,0.00013,-15.0);
        glScalef(1.0,1.0,1.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(15.0,0.0,1);  glVertex3f(-6,1.0,7.1);//left_depan
            glTexCoord3f(0.0,0.0,-1);  glVertex3f(-6,1.0,3);//left_belakang
            glTexCoord3f(0.0,15.0,-1);  glVertex3f(21.7,1.0,3);//right_belakang
            glTexCoord3f(15.0,15.0,1);  glVertex3f(21.7,1.0,7.1);//right_depan
        glEnd();
    glPopMatrix();

}

void drawDoubleDoor()
{
    //main door entrance
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureDoorR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(-0.70,0.0,1.0);
        glScalef(xScale,yScale,1);
        glTranslatef(mainDoorRight,0.0,0.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(0.95,0.0,-1.0);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(2.2,0.0,-1.0);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(2.2,-1.5,-1.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(0.95,-1.5,-1.0);
        glEnd();
        glPopMatrix();

    //wall_b left
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureDoorL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(-1.95,0.0,1.0);
        glScalef(xScale,yScale,1);
        glTranslatef(mainDoorLeft,0.0,0.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(0.95,0.0,-1.0);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(2.2,0.0,-1.0);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(2.2,-1.5,-1.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(0.95,-1.5,-1.0);
        glEnd();
        glPopMatrix();

    //exit door
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureDoorL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(1.4,0.0,1.001);
        glScalef(xScale,yScale,1);
        glTranslatef(exitDoorLeft,0.0,0.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(0.75,0.0,-1.0);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(2.2,0.0,-1.0);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(2.2,-1.5,-1.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(0.75,-1.5,-1.0);
        glEnd();
        glPopMatrix();

    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureDoorR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(2.78,0.0,1.0);
        glScalef(xScale,yScale,1);
        glTranslatef(exitDoorRight,0.0,0.0);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(0.75,0.0,-1.0);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(2.2,0.0,-1.0);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(2.2,-1.5,-1.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(0.75,-1.5,-1.0);
        glEnd();
        glPopMatrix();
}

void drawFlower()
{
    //pot left
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(7.0,-1.20,-9.50);
        glScaled(0.15, 0.60, 0.60);
        glColor3f(0.95, 0.55, 0.55);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

    //pot right
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(7.70,-1.20,-9.50);
        glScaled(0.15, 0.60, 0.60);
        glColor3f(0.95, 0.55, 0.55);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

    //pot behind
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(7.35,-1.20,-9.75);
        glScaled(0.60, 0.60, 0.15);
        glColor3f(0.95, 0.55, 0.55);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

    //pot behind
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(7.35,-1.20,-9.25);
        glScaled(0.60, 0.60, 0.15);
        glColor3f(0.95, 0.55, 0.55);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();



    //flow mid
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureSky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(7.015,-0.15,-8.40);
        glScalef(0.20,1.0,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(1.5,0.3,-1.0);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(2.0,0.3,-1.0);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(2.0,-4.2,-1.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(1.5,-4.2,-1.0);
        glEnd();
        glPopMatrix();

    //flow mid
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureSky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(7.015,-0.15,-8.45);
        glScalef(0.20,1.0,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(1.5,0.3,-1.0);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(2.0,0.3,-1.0);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(2.0,-4.2,-1.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(1.5,-4.2,-1.0);
        glEnd();
        glPopMatrix();

        //flow mid
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureSky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(7.20,-0.15,-10.0);
        glScalef(0.20,1.0,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(1.0,0.3,0.6);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(1.0,0.3,0.50);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(1.0,-4.2,0.50);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(1.0,-4.2,0.6);
        glEnd();
        glPopMatrix();

    //flow right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureSky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(7.0,-0.10,-8.40);
        glScalef(0.20,1.0,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(2.8,0.2,-1.0);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(3.0,0.1,-1.0);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(1.2,-0.2,-1.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(0.5,-0.2,-1.0);
        glEnd();
        glPopMatrix();

    //flow left
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureSky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(7.75,-0.10,-8.40);
        glScalef(0.20,1.0,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(-2.8,0.2,-1.0);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(-3.0,0.1,-1.0);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(-1.2,-0.2,-1.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(-0.5,-0.2,-1.0);
        glEnd();
        glPopMatrix();
}

void drawRuko()
{
    //lemari back
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureGrey);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(2.50,0.0,-10.70);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(1.5,-0.30,-1.0);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(3.8,-0.30,-1.0);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(3.8,-1.5,-1.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(1.5,-1.5,-1.0);
        glEnd();
        glPopMatrix();

        //lemari top/bot
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureSky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(4.30,0.0,-11.2);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(-0.30,-1.0,0.1);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(-0.30,-1.0,-0.5);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(2.0,-1.0,-0.5);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(2.0,-1.0,0.1);
        glEnd();
        glPopMatrix();

        //lemari top/bot
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureSky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(4.30,0.0,-11.2);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,1.0,0.0);     glVertex3f(-0.30,-0.50,0.1);
            glTexCoord3f(1.0,1.0,0.0);     glVertex3f(-0.30,-0.50,-0.5);
            glTexCoord3f(1.0,0.0,0.0);      glVertex3f(2.0,-0.50,-0.5);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(2.0,-0.50,0.1);
        glEnd();
        glPopMatrix();


       //sofa_back
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(0.3,-1.0,-11.8);
        glScaled(2.5, 1.0, 0.1);
        glColor3f(0.95, 0.95, 0.95);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

        //sofa_crouch
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(0.3,-1.3,-11.5);
        glScaled(2.5, 0.50, 0.6);
        glColor3f(1.0, 0.35, 0.35);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

        //sofa_pillow
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(1.65,-1.0,-11.5);
        glScaled(0.20, 0.50, 0.6);
        glColor3f(1.0, 0.35, 0.35);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

        //sofa_back
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(-0.88,-1.0,-10.6);
        glScaled(0.15, 1.0, 2.3);
        glColor3f(0.95, 0.95, 0.95);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

        //sofa_crouch
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(-0.5,-1.3,-10.6);
        glScaled(0.6, 0.50, 2.3);
        glColor3f(1.0, 0.35, 0.35);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

        //sofa_pillow
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(-0.65,-1.0,-9.35);
        glScaled(0.50, 0.6, 0.20);
        glColor3f(1.0, 0.35, 0.35);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();


        //lamp
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(3.0,-1.0,-10.6);
        glScaled(0.1, 1.0, 0.1);
        glColor3f(0.70, 0.35, 0.35);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

        //lamp
        glPushMatrix();
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glPushMatrix();
        glTranslatef(3.0,-0.50,-10.6);
        glScaled(0.35, 0.40, 0.35);
        glColor3f(1.0, 1.0, 0.5);
        glutSolidCube(1);
        glPopMatrix();
        glPopMatrix();

        //wall_b right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(5.0,0.0,-13.50);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,2.0,0.0);     glVertex3f(-1.0,1.0,1.5);
            glTexCoord3f(2.0,2.0,0.0);     glVertex3f(-1.0,1.0,5.0);
            glTexCoord3f(2.0,0.0,0.0);      glVertex3f(-1.0,-1.5,5.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(-1.0,-1.5,1.5);
        glEnd();
        glPopMatrix();

        //wall_b right
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureWallInside);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-6);
        glRotatef(angle, 0.0,1, zangle);
        glTranslatef(10.0,0.0,-13.50);
        glScalef(xScale,yScale,1);
        glBegin(GL_QUADS);
        glColor3f(0.75,0.75,0.75);
            glTexCoord3f(0.0,2.0,0.0);     glVertex3f(-1.0,1.0,1.5);
            glTexCoord3f(2.0,2.0,0.0);     glVertex3f(-1.0,1.0,5.0);
            glTexCoord3f(2.0,0.0,0.0);      glVertex3f(-1.0,-1.5,5.0);
            glTexCoord3f(0.0,0.0,0.0);      glVertex3f(-1.0,-1.5,1.5);
        glEnd();
        glPopMatrix();

        //inside ruko
        drawFlower();
}

void drawBaseSecondLevel()
{
    //wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(0.0,2.50,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(4.0,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(4.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();

    glPopMatrix();
    //end

    //wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(6.0,2.50,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(4.0,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(4.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();

    glPopMatrix();
    //end

    //wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(12.0,2.50,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(4.0,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(4.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();

    glPopMatrix();
    //end

    //wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(18.0,2.50,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(4.0,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(4.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();

    glPopMatrix();
    //end

    //wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(24.0,2.50,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(1.90,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(1.90,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();


    glPopMatrix();
    //end

    /////////////////

    //wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(0.0,5.0,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(4.0,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(4.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();

    glPopMatrix();
    //end

    //wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(6.0,5.0,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(4.0,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(4.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();

    glPopMatrix();
    //end

    //wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(12.0,5.0,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(4.0,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(4.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();

    glPopMatrix();
    //end

    //wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(18.0,5.0,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(4.0,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(4.0,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();
    glPopMatrix();
    //end

    //wall_dd front wall outerlayer
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, _textureOuterLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTranslatef(0,0,-6);
    glRotatef(angle, 0.0,1, zangle);
    glTranslatef(24.0,5.0,0.0);
    glScalef(xScale,yScale,1);
    glBegin(GL_QUADS);
    glColor3f(0.75,0.75,0.75);
        glTexCoord3f(0.0,2,0.1);    glVertex3f(-2.0,1.0,1.0);
        glTexCoord3f(5,2,0.1);      glVertex3f(1.90,1.0,1.0);
        glTexCoord3f(5,0.0,0.1);    glVertex3f(1.90,-1.58,1.0);
        glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-2.0,-1.58,1.0);
    glEnd();

    //wall_dd windows
    glBindTexture(GL_TEXTURE_2D, _textureWindow1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(1.6,0.75,0.0);
        glScalef(1.6,2.5,1);
        glBegin(GL_QUADS);  // Window Left
            glTexCoord3f(0.0,1.0,1.0001); glVertex3f(-1.5,-0.3,1.0001);
            glTexCoord3f(1.0,1.0,1.0001); glVertex3f(-0.75,-0.3,1.0001);
            glTexCoord3f(1.0,0.0,1.0001); glVertex3f(-0.75,-0.8,1.0001);
            glTexCoord3f(0.0,0.0,1.0001); glVertex3f(-1.5,-0.8,1.0001);
        glEnd();


    glPopMatrix();
    //end
}

void drawBase()
{
    drawBase_Front();
    drawBase_Right();
    drawBase_Left();
    drawBase_Behind();

    //levels
    drawBaseSecondLevel();
}

void renderScene(void) {
    setupLights();

    if (is_depth)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    else
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);

    //sky
    drawSky();

    //street
    drawStreet();

    //floor
    drawFloor();

////////////////////////front wall//////////////////////////////////////////////////
    drawBase();
////////////////////////interior/////////////////////////////////////////////////
    drawStairs();
    drawDoubleDoor();
////////////////////////////////////////////////////////////////////////////////////

    //
    drawRuko();

/////////////////////////////
    glColor3f(1.0,1.0,1.0);
    glutSwapBuffers();

}

void keyboardFunc(unsigned char key, int x, int y){
	switch (key)
    {
    case 'w':
    case 'W':
        glTranslatef(0.0,0.0,1.0);
        break;
    case 'a':
    case 'A':
        glTranslatef(1.0,0.0,0.0);
        break;
    case 's':
    case 'S':
        glTranslatef(0.0,0.0,-1.0);
        break;
    case 'd':
    case 'D':
        glTranslatef(-1.0,0.0,0.0);
        break;
    case '7':
        glTranslatef(0.0,1.0,0.0);
        break;
    case '9':
        glTranslatef(0.0,-1.0,0.0);
        break;
    case '2':
        glRotatef(2.0,1.0,0.0,0.0);
        break;
    case '8':
        glRotatef(-2.0,1.0,0.0,0.0);
        break;
    case '6':
        glRotatef(-2.0,0.0,1.0,0.0);
        break;
    case '1':
        glRotatef(2.0,0.0,0.0,1.0);
        break;
    case '3':
        glRotatef(-2.0,0.0,0.0,1.0);
        break;
    case '5':
        if (is_depth)
        {
            is_depth=0;
            glDisable(GL_DEPTH_TEST);
        }
        else
        {
            is_depth=1;
            glEnable(GL_DEPTH_TEST);
        }

    //to open and close the main door
	case 'z':
	case 'Z':
				if(mainDoorLeft == 0 && mainDoorRight == 0)
				  mainDoorLeft=-1,mainDoorRight=1;
				else
					mainDoorLeft=0,mainDoorRight=0;
	break;

	//to open and close the exit door
	case 'x':
	case 'X':
				if(exitDoorLeft == 0 && exitDoorRight == 0)
				  exitDoorLeft=-1,exitDoorRight=1;
				else
					exitDoorLeft=0,exitDoorRight=0;
	break;

    }
	glutPostRedisplay();
}

void mySpecialFunc(int key, int x, int y){
	switch (key) {
    case GLUT_KEY_RIGHT:
        angle -= 1;
        if (angle > 360) angle = 0.0;
		break;
    case GLUT_KEY_LEFT:
        angle += 1;
        if (angle > 360) angle = 0.0;
	    break;
    case GLUT_KEY_UP:
        glTranslatef(0.0,-1.0,0.0);
        break;
    case GLUT_KEY_DOWN:
        glTranslatef(0.0,1.0,0.0);
        break;

	}
	glutPostRedisplay();
}

//texture
GLuint loadTexture(Image* image) {
	GLuint textureId;
	glGenTextures(1, &textureId); //Make room for our texture
	glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit
	//Map the image to the texture
	glTexImage2D(GL_TEXTURE_2D,                //Always GL_TEXTURE_2D
				 0,                            //0 for now
				 GL_RGB,                       //Format OpenGL uses for image
				 image->width, image->height,  //Width and height
				 0,                            //The border of the image
				 GL_RGB, //GL_RGB, because pixels are stored in RGB format
				 GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
				                   //as unsigned numbers
				 image->pixels);               //The actual pixel data
	return textureId; //Returns the id of the texture
}

void Initialize() {
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	//source texture
	Image*
	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\sky.bmp");
	_textureSky = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\street.bmp");
	_textureStreet = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\outerlayer.bmp");
	_textureOuterLayer = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\wallinside.bmp");
	_textureWallInside = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\window1.bmp");
	_textureWindow1 = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\grey.bmp");
	_textureGrey = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\logo1.bmp");
	_textureLogo1 = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\door2.bmp");
	_texturedoor2 = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\logo2.bmp");
	_textureLogo2 = loadTexture(image);

		image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\logo3.bmp");
	_textureLogo3 = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\window2.bmp");
	_textureWindow2 = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\mallfloor.bmp");
	_textureMallFloor = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\stairwall.bmp");
	_textureStairWall = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\doorL.bmp");
	_textureDoorL = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\doorR.bmp");
	_textureDoorR = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\bricks.bmp");
	_textureBrick = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\door.bmp");
	_textureDoor = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\grass.bmp");
	_textureGrass = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\roof.bmp");
	_textureRoof = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Documents\\TR\\TR_GRAFKOM_672018303\\mall\\Offline6\\window.bmp");
	_textureWindow = loadTexture(image);

	delete image;
}

void mouseMove(int x, int y) {

         // this will only be true when the left button is down
         if (xOrigin >= 0) {

		// update deltaAngle
		deltaAngle = (x - xOrigin) * 0.10f;

		// update camera's direction
		lx = sin(angle + deltaAngle);
		lz = -cos(angle + deltaAngle);
	}
}

void mouseButton(int button, int state, int x, int y) {

	// only start motion if the left button is pressed
	if (button == GLUT_LEFT_BUTTON) {

		// when the button is released
		if (state == GLUT_UP) {
			angle += deltaAngle;
			xOrigin = -1;
		}
		else  {// state = GLUT_DOWN
			xOrigin = x;
		}
	}
}

int main(int argc, char **argv) {
    printf("Press Z for Main Door on & off\n");
    printf("Press X for Exit Door on & off");
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(0,0);
	glutInitWindowSize(1366,768);
	glutCreateWindow("TR-GRAFKOM-672018303-672018325");
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
    glutReshapeFunc(resize);
    glutDisplayFunc(renderScene);
	glutKeyboardFunc(keyboardFunc);
	glutSpecialFunc(mySpecialFunc);

	// here are the two new functions
	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMove);

	Initialize();

	glutMainLoop();
    return 0;
}
