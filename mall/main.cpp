#include <stdlib.h>
#include <GL/glut.h>
#include "imageloader.h"
#include <fstream>
#include <assert.h>
using namespace std;

float angle = 0;
float xScale=1,yScale=1,zangle=0;
int is_depth;
GLUquadricObj *Disk;

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

void drawSky()
{
    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, _textureSky);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTranslatef(0,0,-35);
        glBegin(GL_QUADS);
            glTexCoord3f(0.0,1.0,0.1);  glVertex3f(-30,30,0);
            glTexCoord3f(1.0,1.0,0.1);  glVertex3f(30,30,0);
            glTexCoord3f(1.0,0.0,0.1);  glVertex3f(30,-30,0);
            glTexCoord3f(0.0,0.0,0.1);  glVertex3f(-30,-30,0);
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
            glTexCoord3f(15.0,0.0,1);  glVertex3f(-6,-1.5,5);
            glTexCoord3f(0.0,0.0,-1);  glVertex3f(-6,-1.5,-6);
            glTexCoord3f(0.0,15.0,-1);  glVertex3f(21.7,-1.5,-6);
            glTexCoord3f(15.0,15.0,1);  glVertex3f(21.7,-1.5,5);
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
}

void drawBase()
{
    drawBase_Front();
    drawBase_Right();
    drawBase_Left();
    drawBase_Behind();
}

void renderScene(void) {
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
////////////////////////////////////////////////////////////////////////////////////
    drawStairs();
////////////////////////////////////////////////////////////////////////////////////
    glColor3f(1.0,1.0,1.0);
    glutSwapBuffers();

}

void keyboardFunc(unsigned char key, int x, int y){
	switch (key)
    {
    case 'w':
    case 'W':
        glTranslatef(0.0,0.0,3.0);
        break;
    case 'a':
    case 'A':
        glTranslatef(3.0,0.0,0.0);
        break;
    case 's':
    case 'S':
        glTranslatef(0.0,0.0,-3.0);
        break;
    case 'd':
    case 'D':
        glTranslatef(-3.0,0.0,0.0);
        break;
    case '7':
        glTranslatef(0.0,3.0,0.0);
        break;
    case '9':
        glTranslatef(0.0,-3.0,0.0);
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
	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\sky.bmp");
	_textureSky = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\street.bmp");
	_textureStreet = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\outerlayer.bmp");
	_textureOuterLayer = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\wallinside.bmp");
	_textureWallInside = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\window1.bmp");
	_textureWindow1 = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\grey.bmp");
	_textureGrey = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\logo1.bmp");
	_textureLogo1 = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\door2.bmp");
	_texturedoor2 = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\logo2.bmp");
	_textureLogo2 = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\window2.bmp");
	_textureWindow2 = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\mallfloor.bmp");
	_textureMallFloor = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\stairwall.bmp");
	_textureStairWall = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\bricks.bmp");
	_textureBrick = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\door.bmp");
	_textureDoor = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\grass.bmp");
	_textureGrass = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\roof.bmp");
	_textureRoof = loadTexture(image);

	image = loadBMP("C:\\Users\\Asus\\Desktop\\TR_GRAFKOM_672018303\\mall\\graphics\\window.bmp");
	_textureWindow = loadTexture(image);

	delete image;
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(0,0);
	glutInitWindowSize(1366,768);
	glutCreateWindow("TR_GRAFKOM_672018303");
	glEnable(GL_DEPTH_TEST);
    glutReshapeFunc(resize);
    glutDisplayFunc(renderScene);
	glutKeyboardFunc(keyboardFunc);
	glutSpecialFunc(mySpecialFunc);
	Initialize();

	glutMainLoop();
    return 0;
}
