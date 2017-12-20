#include "Render.h"

#include <windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"



bool textureMode = true;
bool lightMode = true;
GLuint texId;
//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	void SetUpCamera()
	{
		//отвечает за поворот камеры мышкой
		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		//функция настройки камеры
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		glDisable(GL_LIGHTING);

		
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();
		
		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}

	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света




//старые координаты мыши
int mouseX = 0, mouseY = 0;

void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}

	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}

	
}

void mouseWheelEvent(OpenGL *ogl, int delta)
{

	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;

}

void keyDownEvent(OpenGL *ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}
}

void keyUpEvent(OpenGL *ogl, int key)
{
	
}




//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{
	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);
	

	//массив трехбайтных элементов  (R G B)
	RGBTRIPLE *texarray;

	//массив символов, (высота*ширина*4      4, потомучто   выше, мы указали использовать по 4 байта на пиксель текстуры - R G B A)
	char *texCharArray;
	int texW, texH;
	OpenGL::LoadBMP("texture.bmp", &texW, &texH, &texarray);
	OpenGL::RGBtoChar(texarray, texW, texH, &texCharArray);

	
	//GLuint texId;
	//генерируем ИД для текстуры
	glGenTextures(1, &texId);
	//биндим айдишник, все что будет происходить с текстурой, будте происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texId);

	//загружаем текстуру в видеопямять, в оперативке нам больше  она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray);

	//отчистка памяти
	free(texCharArray);
	free(texarray);

	//наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
}


void drawSphere(double center[3], double rad) {
	double currentPoint[3];
	double t1 = 0, t2 = 0;
	
	for (double i = 0; i < 179; i++)
	{
		t1 = 0;
		t2 += 1024.0/179;
		glBindTexture(GL_TEXTURE_2D, 1);
		glBegin(GL_TRIANGLE_STRIP);
		for (double j = 1; j < 360; j++)
		{

			currentPoint[0] = center[0] + rad * sinf(i) * cosf(j);
			currentPoint[1] = center[1] + rad * sinf(i) * sinf(j);
			currentPoint[2] = center[2] + rad * cos(i);
			//t1 += 1024.0 / 360;
			glNormal3dv(currentPoint);
			glTexCoord2d(t1 / 1024.0, t2 / 1024.0);
			glVertex3dv(currentPoint);

			currentPoint[0] = center[0] + rad * sinf(i + 1) * cosf(j);
			currentPoint[1] = center[1] + rad * sinf(i + 1) * sinf(j);
			currentPoint[2] = center[2] + rad * cos(i + 1);
			t1 += 1024.0 / 360;
			glNormal3dv(currentPoint);
			glTexCoord2d(t1 / 1024.0, t2 / 1024.0);
			glVertex3dv(currentPoint);
		}
		glEnd();
	}
}

void drawHands(double startLeft[3], double startRight[3]) {
	
	//left
	double A[3] = { startLeft[0] - 1, startLeft[1] - 0.5, startLeft[2] - 2 };
	double B[3] = { startLeft[0] - 1.5, startLeft[1] - 0.5, startLeft[2] - 2 };
	double C[3] = { startLeft[0] - 1.5, startLeft[1] - 0.5, startLeft[2] - 1 };
	double D[3] = { startLeft[0] - 0.5, startLeft[1] - 0.5, startLeft[2] + 0.5 };
	double E[3] = { startLeft[0], startLeft[1] - 0.5, startLeft[2] + 0.5 };
	double F[3] = { startLeft[0], startLeft[1] - 0.5, startLeft[2] };

	double A1[3] = { startLeft[0] - 1, startLeft[1] + 0.5, startLeft[2] - 2 };
	double B1[3] = { startLeft[0] - 1.5, startLeft[1] + 0.5, startLeft[2] - 2 };
	double C1[3] = { startLeft[0] - 1.5, startLeft[1] + 0.5, startLeft[2] - 1 };
	double D1[3] = { startLeft[0] - 0.5, startLeft[1] + 0.5, startLeft[2] + 0.5 };
	double E1[3] = { startLeft[0], startLeft[1] + 0.5, startLeft[2] + 0.5 };
	double F1[3] = { startLeft[0], startLeft[1] + 0.5, startLeft[2] };

	glBegin(GL_POLYGON);
	glNormal3d(0, -1, 0);
	glVertex3dv(A);
	glVertex3dv(B);
	glVertex3dv(C);
	glVertex3dv(D);
	glVertex3dv(E);
	glVertex3dv(F);
	glEnd();
	
	glBegin(GL_POLYGON);
	glNormal3d(0, 1, 0);
	glVertex3dv(A1);
	glVertex3dv(B1);
	glVertex3dv(C1);
	glVertex3dv(D1);
	glVertex3dv(E1);
	glVertex3dv(F1);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glVertex3dv(A);
	glVertex3dv(A1);
	glVertex3dv(B1);
	glVertex3dv(B);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(C);
	glVertex3dv(C1);
	glVertex3dv(B1);
	glVertex3dv(B);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(C);
	glVertex3dv(C1);
	glVertex3dv(D1);
	glVertex3dv(D);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glVertex3dv(D);
	glVertex3dv(D1);
	glVertex3dv(E1);
	glVertex3dv(E);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(E);
	glVertex3dv(E1);
	glVertex3dv(F1);
	glVertex3dv(F);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(A);
	glVertex3dv(A1);
	glVertex3dv(F1);
	glVertex3dv(F);
	glEnd();

	//right
	double Ar[3] = { startRight[0] + 1, startRight[1] - 0.5, startRight[2] - 2 };
	double Br[3] = { startRight[0] + 1.5, startRight[1] - 0.5, startRight[2] - 2 };
	double Cr[3] = { startRight[0] + 1.5, startRight[1] - 0.5, startRight[2] - 1 };
	double Dr[3] = { startRight[0] + 0.5, startRight[1] - 0.5, startRight[2] + 0.5 };
	double Er[3] = { startRight[0], startRight[1] - 0.5, startRight[2] + 0.5 };
	double Fr[3] = { startRight[0], startRight[1] - 0.5, startRight[2] };

	double A1r[3] = { startRight[0] + 1, startRight[1] + 0.5, startRight[2] - 2 };
	double B1r[3] = { startRight[0] + 1.5, startRight[1] + 0.5, startRight[2] - 2 };
	double C1r[3] = { startRight[0] + 1.5, startRight[1] + 0.5, startRight[2] - 1 };
	double D1r[3] = { startRight[0] + 0.5, startRight[1] + 0.5, startRight[2] + 0.5 };
	double E1r[3] = { startRight[0], startRight[1] + 0.5, startRight[2] + 0.5 };
	double F1r[3] = { startRight[0], startRight[1] + 0.5, startRight[2] };

	glBegin(GL_POLYGON);
	glNormal3d(0, -1, 0);
	glVertex3dv(Ar);
	glVertex3dv(Br);
	glVertex3dv(Cr);
	glVertex3dv(Dr);
	glVertex3dv(Er);
	glVertex3dv(Fr);
	glEnd();

	glBegin(GL_POLYGON);
	glNormal3d(0, 1, 0);
	glVertex3dv(A1r);
	glVertex3dv(B1r);
	glVertex3dv(C1r);
	glVertex3dv(D1r);
	glVertex3dv(E1r);
	glVertex3dv(F1r);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glVertex3dv(Ar);
	glVertex3dv(A1r);
	glVertex3dv(B1r);
	glVertex3dv(Br);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(Cr);
	glVertex3dv(C1r);
	glVertex3dv(B1r);
	glVertex3dv(Br);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(Cr);
	glVertex3dv(C1r);
	glVertex3dv(D1r);
	glVertex3dv(Dr);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glVertex3dv(Dr);
	glVertex3dv(D1r);
	glVertex3dv(E1r);
	glVertex3dv(Er);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(Er);
	glVertex3dv(E1r);
	glVertex3dv(F1r);
	glVertex3dv(Fr);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(Ar);
	glVertex3dv(A1r);
	glVertex3dv(F1r);
	glVertex3dv(Fr);
	glEnd();
}

void drawEars(double startLeft[3], double startRight[3]) {
	//left
	double A[3] = { startLeft[0] + 1, startLeft[1] - 0.5, startLeft[2] };
	double B[3] = { startLeft[0], startLeft[1] - 0.5, startLeft[2] };
	double C[3] = { startLeft[0] - 1, startLeft[1] - 1, startLeft[2] + 5 };
	double D[3] = { startLeft[0] - 1, startLeft[1] - 2, startLeft[2] + 7 };
	double E[3] = { startLeft[0], startLeft[1] - 2, startLeft[2] + 7 };
	double F[3] = { startLeft[0] + 0.5, startLeft[1] - 1, startLeft[2]  + 5};

	double A1[3] = { startLeft[0] + 1, startLeft[1], startLeft[2] };
	double B1[3] = { startLeft[0], startLeft[1], startLeft[2] };
	double C1[3] = { startLeft[0] - 1, startLeft[1] - 0.5, startLeft[2] + 5 };
	double D1[3] = { startLeft[0] - 1, startLeft[1] - 1.5, startLeft[2] + 7 };
	double E1[3] = { startLeft[0], startLeft[1] - 1.5, startLeft[2] + 7 };
	double F1[3] = { startLeft[0] + 0.5, startLeft[1] - 0.5, startLeft[2] + 5 };

	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glVertex3dv(A);
	glVertex3dv(B);
	glVertex3dv(C);
	glVertex3dv(F);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glVertex3dv(E);
	glVertex3dv(D);
	glVertex3dv(C);
	glVertex3dv(F);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glVertex3dv(A1);
	glVertex3dv(B1);
	glVertex3dv(C1);
	glVertex3dv(F1);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glVertex3dv(E1);
	glVertex3dv(D1);
	glVertex3dv(C1);
	glVertex3dv(F1);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glVertex3dv(A);
	glVertex3dv(A1);
	glVertex3dv(B1);
	glVertex3dv(B);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(C);
	glVertex3dv(C1);
	glVertex3dv(B1);
	glVertex3dv(B);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(C);
	glVertex3dv(C1);
	glVertex3dv(D1);
	glVertex3dv(D);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glVertex3dv(D);
	glVertex3dv(D1);
	glVertex3dv(E1);
	glVertex3dv(E);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(E);
	glVertex3dv(E1);
	glVertex3dv(F1);
	glVertex3dv(F);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(A);
	glVertex3dv(A1);
	glVertex3dv(F1);
	glVertex3dv(F);
	glEnd();

	//right
	double Ar[3] = { startRight[0] - 1, startRight[1] - 0.5, startRight[2] };
	double Br[3] = { startRight[0], startRight[1] - 0.5, startRight[2] };
	double Cr[3] = { startRight[0] + 1, startRight[1] - 1, startRight[2] + 5 };
	double Dr[3] = { startRight[0] + 1, startRight[1] - 2.0, startRight[2] + 7 };
	double Er[3] = { startRight[0], startRight[1] - 2.0, startRight[2] + 7 };
	double Fr[3] = { startRight[0] - 0.5, startRight[1] - 1, startRight[2] + 5 };

	double A1r[3] = { startRight[0] - 1, startRight[1], startRight[2] };
	double B1r[3] = { startRight[0], startRight[1], startRight[2] };
	double C1r[3] = { startRight[0] + 1, startRight[1] - 0.5, startRight[2] + 5 };
	double D1r[3] = { startRight[0] + 1, startRight[1] - 1.5, startRight[2] + 7 };
	double E1r[3] = { startRight[0], startRight[1] - 1.5, startRight[2] + 7 };
	double F1r[3] = { startRight[0] - 0.5, startRight[1] - 0.5, startRight[2] + 5 };

	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glVertex3dv(Ar);
	glVertex3dv(Br);
	glVertex3dv(Cr);
	glVertex3dv(Fr);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glVertex3dv(Er);
	glVertex3dv(Dr);
	glVertex3dv(Cr);
	glVertex3dv(Fr);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glVertex3dv(A1r);
	glVertex3dv(B1r);
	glVertex3dv(C1r);
	glVertex3dv(F1r);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glVertex3dv(E1r);
	glVertex3dv(D1r);
	glVertex3dv(C1r);
	glVertex3dv(F1r);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glVertex3dv(Ar);
	glVertex3dv(A1r);
	glVertex3dv(B1r);
	glVertex3dv(Br);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(Cr);
	glVertex3dv(C1r);
	glVertex3dv(B1r);
	glVertex3dv(Br);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(Cr);
	glVertex3dv(C1r);
	glVertex3dv(D1r);
	glVertex3dv(Dr);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glVertex3dv(Dr);
	glVertex3dv(D1r);
	glVertex3dv(E1r);
	glVertex3dv(Er);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(Er);
	glVertex3dv(E1r);
	glVertex3dv(F1r);
	glVertex3dv(Fr);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(Ar);
	glVertex3dv(A1r);
	glVertex3dv(F1r);
	glVertex3dv(Fr);
	glEnd();
}

void drawLegs(double startLeft[3], double startRight[3]) {
	//left

	double A[3] = { startLeft[0] + 1, startLeft[1] + 0.5, startLeft[2] - 2 };
	double B[3] = { startLeft[0] - 1.5, startLeft[1] + 0.5, startLeft[2] - 2 };
	double C[3] = { startLeft[0] - 1.5, startLeft[1] - 1.5, startLeft[2] - 2 };
	double D[3] = { startLeft[0] + 1, startLeft[1] - 1.5, startLeft[2] - 2 };

	double A1[3] = { startLeft[0] + 1, startLeft[1] + 0.5, startLeft[2] - 1 };
	double B1[3] = { startLeft[0] - 1.5, startLeft[1] + 0.5, startLeft[2] - 1 };
	double C1[3] = { startLeft[0] - 1.5, startLeft[1] - 1.5, startLeft[2] - 1 };
	double D1[3] = { startLeft[0] + 1, startLeft[1] - 1.5, startLeft[2] - 1 };

	double A2[3] = { startLeft[0] + 1, startLeft[1], startLeft[2] };
	double B2[3] = { startLeft[0], startLeft[1], startLeft[2] };
	double C2[3] = { startLeft[0], startLeft[1] - 0.5, startLeft[2] };
	double D2[3] = { startLeft[0] + 1, startLeft[1] - 0.5, startLeft[2] };

	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glVertex3dv(A);
	glVertex3dv(B);
	glVertex3dv(C);
	glVertex3dv(D);
	glEnd();
	glBegin(GL_QUADS);
	glVertex3dv(A2);
	glVertex3dv(B2);
	glVertex3dv(C2);
	glVertex3dv(D2);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glVertex3dv(A);
	glVertex3dv(B);
	glVertex3dv(B1);
	glVertex3dv(A1);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(C);
	glVertex3dv(B);
	glVertex3dv(B1);
	glVertex3dv(C1);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glVertex3dv(C);
	glVertex3dv(D);
	glVertex3dv(D1);
	glVertex3dv(C1);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(A);
	glVertex3dv(D);
	glVertex3dv(D1);
	glVertex3dv(A1);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glVertex3dv(A1);
	glVertex3dv(B1);
	glVertex3dv(B2);
	glVertex3dv(A2);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(C1);
	glVertex3dv(B1);
	glVertex3dv(B2);
	glVertex3dv(C2);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glVertex3dv(C1);
	glVertex3dv(D1);
	glVertex3dv(D2);
	glVertex3dv(C2);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(A1);
	glVertex3dv(D1);
	glVertex3dv(D2);
	glVertex3dv(A2);
	glEnd();

	//right

	double Ar[3] = { startRight[0] - 1, startRight[1] + 0.5, startRight[2] - 2 };
	double Br[3] = { startRight[0] + 1.5, startRight[1] + 0.5, startRight[2] - 2 };
	double Cr[3] = { startRight[0] + 1.5, startRight[1] - 1.5, startRight[2] - 2 };
	double Dr[3] = { startRight[0] - 1, startRight[1] - 1.5, startRight[2] - 2 };

	double A1r[3] = { startRight[0] - 1, startRight[1] + 0.5, startRight[2] - 1 };
	double B1r[3] = { startRight[0] + 1.5, startRight[1] + 0.5, startRight[2] - 1 };
	double C1r[3] = { startRight[0] + 1.5, startRight[1] - 1.5, startRight[2] - 1 };
	double D1r[3] = { startRight[0] - 1, startRight[1] - 1.5, startRight[2] - 1 };

	double A2r[3] = { startRight[0] - 1, startRight[1], startRight[2] };
	double B2r[3] = { startRight[0], startRight[1], startRight[2] };
	double C2r[3] = { startRight[0], startRight[1] - 0.5, startRight[2] };
	double D2r[3] = { startRight[0] - 1, startRight[1] - 0.5, startRight[2] };

	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glVertex3dv(Ar);
	glVertex3dv(Br);
	glVertex3dv(Cr);
	glVertex3dv(Dr);
	glEnd();
	glBegin(GL_QUADS);
	glVertex3dv(A2r);
	glVertex3dv(B2r);
	glVertex3dv(C2r);
	glVertex3dv(D2r);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glVertex3dv(Ar);
	glVertex3dv(Br);
	glVertex3dv(B1r);
	glVertex3dv(A1r);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(Cr);
	glVertex3dv(Br);
	glVertex3dv(B1r);
	glVertex3dv(C1r);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glVertex3dv(Cr);
	glVertex3dv(Dr);
	glVertex3dv(D1r);
	glVertex3dv(C1r);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(Ar);
	glVertex3dv(Dr);
	glVertex3dv(D1r);
	glVertex3dv(A1r);
	glEnd();

	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glVertex3dv(A1r);
	glVertex3dv(B1r);
	glVertex3dv(B2r);
	glVertex3dv(A2r);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(C1r);
	glVertex3dv(B1r);
	glVertex3dv(B2r);
	glVertex3dv(C2r);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glVertex3dv(C1r);
	glVertex3dv(D1r);
	glVertex3dv(D2r);
	glVertex3dv(C2r);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(A1r);
	glVertex3dv(D1r);
	glVertex3dv(D2r);
	glVertex3dv(A2r);
	glEnd();

}

void sphere(float R, int s2, int s1, int m, int l, bool f, double center[3])
{//радиус и количество полигонов в разные плоскости,на склько частей резать сферу 
	float x1, y1, z1, x2, y2, z2;
	float a = 1, a2 = 1;
	double t1 = 0, t2 = 0;
	//glColor3dv(rgb1); 
	glColor3f(1, 1, 1);
	for (int i = 0; i < s1 / m; i++)//что бы обрезать сферу делим s1 на 2 если надо полусферу,на 3 если надо треть и т д 
	{
		t1 = 0;
		t2 += 1024.0 / s1;
		a2 = (i*M_PI * 2) / (s1 / m);
		glBindTexture(GL_TEXTURE_2D, texId);
		glBegin(GL_TRIANGLE_STRIP);
		for (int j = 0; j <= s2 / l; j++)
		{//что бы обрезать сферу делим s2 на 2 если надо полусферу,на 3 если надо треть и т д 

			x1 = center[0] + R * sin(-2 * M_PI * j / s2) * sin(M_PI * i / s1);
			x2 = center[0] + R * sin(-2 * M_PI * j / s2) * sin(M_PI * (i + 1) / s1);
			y1 = center[1] + R * cos(-2 * M_PI * j / s2) * sin(M_PI * i / s1);
			y2 = center[1] + R * cos(-2 * M_PI * j / s2) * sin(M_PI * (i + 1) / s1);
			z1 = center[2] + R * cos(M_PI * i / s1);
			z2 = center[2] + R * cos(M_PI * (i + 1) / s1);
			if (f == true) {
				a2 = (i*M_PI * 2) / (s1 / m);
			}
			else {
				a2 = ((i + 1)*M_PI * 2) / (s1 / m);
			}
			t1 += 1024.0 / s2;
			glNormal3d(x1 / R * sin(M_PI * i / s1), y1 / R * sin(M_PI * i / s1), z1 / R);
			glTexCoord2d(t1 / 1024.0, t2 / 1024.0);
			glVertex3d(x1, y1, z1);


			if (f == true) {
				a2 = ((i + 1)*M_PI * 2) / (s1 / m);
			}
			else {
				a2 = (i*M_PI * 2) / (s1 / m);
			}
			glNormal3d(x2 / R * sin(M_PI * (i + 1) / s1), y2 / R * sin(M_PI * (i + 1) / s1), z2 / R);
			glTexCoord2d(t1 / 1024.0, t2 / 1024.0);
			glVertex3d(x2, y2, z2);
		}
		glEnd();
	}

}

void Render(OpenGL *ogl)
{       	
	
	
	
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);


	//альфаналожение
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;


	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);\
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

    //чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH);
	//===================================
	//Прогать тут  

	double center[3] = { 0, 0, 5 };
	//drawSphere(center, 3);
	sphere(3.0, 200, 200, 1, 1, true, center);

	double leftHand[3] = { -2.8, 0, 5 }; 
	double rightHand[3] = { 2.8, 0, 5 };
	drawHands(leftHand, rightHand);
    
	double leftEar[3] = { -1, 0, 7.5};
	double rightEar[3] = { 1, 0, 7.5};
	drawEars(leftEar, rightEar);

	double leftLeg[3] = { -1.2, 0.5, 2.2};
	double rightLeg[3] = { 1.2, 0.5, 2.2};
	drawLegs(leftLeg, rightLeg);
	
	//текст сообщения вверху слева, если надоест - закоментировать, или заменить =)
	char c[250];  //максимальная длина сообщения
	sprintf_s(c, "(T)Текстуры - %d\n(L)Свет - %d\n\nУправление светом:\n"
		"G - перемещение в горизонтальной плоскости,\nG+ЛКМ+перемещение по вертикальной линии\n"
		"R - установить камеру и свет в начальное положение\n"
		"F - переместить свет в точку камеры", textureMode, lightMode);
	ogl->message = std::string(c);




}   //конец тела функции

