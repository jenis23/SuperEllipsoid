
#define GL_GLEXT_PROTOTYPES

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "matrix.h"
#if defined(__APPLE__) || defined(MACOSX)
    #include <OpenGL/gl.h>
    #include <GLUT/glut.h>
#else
    #include <GL/gl.h>
    #include <GL/glut.h>
#endif

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define BIG_M 40
#define BIG_N 24

#define VERTEX_INDEX(i,j) ((i) * (BIG_M + 1) + (j))

#define NUM_STRIPS BIG_N
#define RADIANS_PER_PIXEL M_PI/(2*90.0)
#define INDICES_PER_STRIP (2*BIG_M + 2)
#define TOTAL_INDICES (BIG_N * (2 * (BIG_M + 1) + 4)) - 4

/**
Authors: Jenis Modi
        Wayne Cochran

**/


GLushort tubeStrips[TOTAL_INDICES];
GLfloat texCoordArray[(BIG_M+1) * (BIG_N+1)][2];
double centerX=0,centerY=0,centerZ=0;
double upX=0,upY=0,upZ=1;

GLfloat center;

double eyeX,eyeY,eyeZ;

int mouseX,mouseY;

double phi = 100,radius=5,theta=600;

GLboolean changeFlag;

GLfloat SMALL_M,SMALL_N;

GLfloat verticesArray[3 * (BIG_M + 1) * (BIG_N + 1)];
GLfloat normalsArray[3 * (BIG_M + 1) * (BIG_N + 1)];
/*
GLushort indicesArray[] = {
0, 5, 1, 6, 2, 7, 3, 8, 4, 9, 9, 9,
5, 5, 5, 10, 6, 11, 7, 12, 8, 13, 9, 14, 14, 14, 
10, 10, 10, 15, 11, 16, 12, 17, 13, 18, 14, 19, 19, 19, 
15, 15, 15, 20, 16, 21, 17, 22, 18, 23, 19, 24 
};
*/
void createMeshStripIndices();
void setCamera();



GLfloat ModelView[4*4];
GLfloat Projection[4*4];
GLfloat TextureMatrix[4*4];

GLfloat ambientLight[3] = {0.2, 0.2, 0.2};
GLfloat light0Color[3] = {1.0, 1.0, 1.0};
GLfloat light0Position[3] = {10.0, 10.0, 10.0};

GLfloat materialAmbient[3] = {0.1,0.1,0.1};
GLfloat materialDiffuse[3] = {1.0,1.0,1.0};
GLfloat materialSpecular[3] = {0.3,0.3,0.3};
GLfloat materialShininess = 20.0;

GLint vertexPositionAttr;
GLint vertexNormalAttr;
GLint vertexTexCoordAttr;

GLint ModelViewProjectionUniform;
GLint ModelViewMatrixUniform;
GLint NormalMatrixUniform;
GLint TextureMatrixUniform;

GLint ambientLightUniform;
GLint light0ColorUniform;
GLint light0PositionUniform;

GLint materialAmbientUniform;
GLint materialDiffuseUniform;
GLint labelDiffuseUniform;
GLint materialSpecularUniform;
GLint materialShininessUniform;

GLint texUnitUniform;

GLuint vertexShader;
GLuint fragmentShader;
GLuint program;

GLint texUnitUniform;

void checkOpenGLError(int line) {
    bool wasError = false;
    GLenum error = glGetError();
    while (error != GL_NO_ERROR) {
        printf("GL ERROR: at line %d: %s\n", line, gluErrorString(error));
        wasError = true;
        error = glGetError();
    }
    if (wasError) exit(-1);
}

GLuint texName;
void loadTexture(void) {
    const char *fname = "earth.ppm";
   // const char *fname = "nasadiwali.ppm";
    FILE *f = fopen(fname, "rb");
    if (f == NULL) {
        perror(fname); exit(-1);
    }
    char buf[50];
    if (fgets(buf, sizeof(buf), f) == NULL || strcmp("P6\n",buf) != 0) {
        fprintf(stderr, "%s not a raw PPM file!\n", fname);
        exit(-1);
    }
    while (fgets(buf, sizeof(buf), f) != NULL && buf[0] == '#')
        ;
    int W, H;
    if (sscanf(buf, "%d %d", &W, &H) != 2 || W <= 0 || H <= 0) {
        fprintf(stderr, "%s has bogus size!\n", fname);
        exit(-1);
    }
    while (fgets(buf, sizeof(buf), f) != NULL && buf[0] == '#')
        ;
    int maxval;
    if (sscanf(buf, "%d", &maxval) != 1 || maxval != 255) {
        fprintf(stderr, "%s has bogus max channel value!\n", fname);
        exit(-1);
    }
    GLubyte *pixels = (GLubyte *) malloc(W*H*3); 
    for (int r = H-1; r >= 0; r--) {   // flip upside down
        GLubyte *p = pixels + 3*W*r;
        if (fread(p, 3, W, f) != W) {
            fprintf(stderr, "%s : unable to read pixels!\n", fname);
            exit(-1);
        }
    }
    fclose(f);

    // XXX glPixelStorei(GL_PACK_ALIGNMENT, 1);    // XXX from GL to client
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // from client to GL

    glGenTextures(1, &texName);
    glBindTexture(GL_TEXTURE_2D, texName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, 
                 GL_RGB, GL_UNSIGNED_BYTE, pixels);

    free(pixels);
}



void loadUniforms() {
    //
    // Load Matrices
    //

    GLfloat ModelViewProjection[4*4], NormalMatrix[3*3];
    matrixMultiply(ModelViewProjection, Projection, ModelView);
    matrixNormal(ModelView, NormalMatrix);
    glUniformMatrix4fv(ModelViewProjectionUniform, 1, GL_FALSE,
                       ModelViewProjection);
    glUniformMatrix4fv(ModelViewMatrixUniform, 1, GL_FALSE,
                       ModelView);
    glUniformMatrix3fv(NormalMatrixUniform, 1, GL_FALSE, NormalMatrix);
    glUniformMatrix4fv(TextureMatrixUniform, 1, GL_FALSE, TextureMatrix);

    //
    // Load lights.
    //
    glUniform3fv(ambientLightUniform, 1, ambientLight);
    glUniform3fv(light0ColorUniform, 1, light0Color);
    glUniform3fv(light0PositionUniform, 1, light0Position);

    //
    // Load material properties.
    //
    glUniform3fv(materialAmbientUniform, 1, materialAmbient);
    glUniform3fv(materialDiffuseUniform, 1, materialDiffuse);
    //  glUniform3fv(labelDiffuseUniform, 1, labelDiffuse);
    glUniform3fv(materialSpecularUniform, 1, materialSpecular);
    glUniform1f(materialShininessUniform, materialShininess);

    //
    // Only using texture unit 0.
    //
    glUniform1i(texUnitUniform, 0);

}

GLchar *getShaderSource(const char *fname) {
    FILE *f = fopen(fname, "r");
    if (f == NULL) {
        perror(fname); exit(-1);
    }
    fseek(f, 0L, SEEK_END);
    int len = ftell(f);
    rewind(f);
    GLchar *source = (GLchar *) malloc(len + 1);
    if (fread(source,1,len,f) != len) {
        if (ferror(f))
            perror(fname);
        else if (feof(f))
            fprintf(stderr, "Unexpected EOF when reading '%s'!\n", fname);
        else
            fprintf(stderr, "Unable to load '%s'!\n", fname);
        exit(-1);
    }
    source[len] = '\0';
    fclose(f);
    return source;
}

//
// Install our shader programs and tell GL to use them.
// We also initialize the uniform variables.
//
void installShaders(void) {
    //
    // (1) Create shader objects
    //
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    //
    // (2) Load source code into shader objects.
    //
    const GLchar *vertexShaderSource = getShaderSource("vertex.vs");
    const GLchar *fragmentShaderSource = getShaderSource("fragment-tex.fs");

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);

    //
    // (3) Compile shaders.
    //
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[200];
        GLint charsWritten;
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), &charsWritten, infoLog);
        fprintf(stderr, "vertex shader info log:\n%s\n\n", infoLog);
    }
    checkOpenGLError(__LINE__);

    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[200];
        GLint charsWritten;
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), &charsWritten, infoLog);
        fprintf(stderr, "fragment shader info log:\n%s\n\n", infoLog);
    }
    checkOpenGLError(__LINE__);

    //
    // (4) Create program object and attach vertex and fragment shader.
    //
    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    checkOpenGLError(__LINE__);

    //
    // (5) Link program.
    //
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[200];
        GLint charsWritten;
        glGetProgramInfoLog(program, sizeof(infoLog), &charsWritten, infoLog);
        fprintf(stderr, "program info log:\n%s\n\n", infoLog);
    }
    checkOpenGLError(__LINE__);

    //
    // (7) Get vertex attribute locations
    //
    vertexPositionAttr = glGetAttribLocation(program, "vertexPosition");
    vertexNormalAttr = glGetAttribLocation(program, "vertexNormal");
    vertexTexCoordAttr = glGetAttribLocation(program, "vertexTexCoord");

    if (vertexPositionAttr == -1 || vertexNormalAttr == -1 || vertexTexCoordAttr == -1) {
        fprintf(stderr, "Error fetching vertex position or normal attribute or texture Attribute!\n");

    }

    //
    // (8) Fetch handles for uniform variables in program.
    //
    ModelViewProjectionUniform = glGetUniformLocation(program, "ModelViewProjection");
    if (ModelViewProjectionUniform == -1) {
        fprintf(stderr, "Error fetching modelViewProjectionUniform  	!\n");
        //    exit(-1);
    }

    ModelViewMatrixUniform = glGetUniformLocation(program, "ModelViewMatrix");

    if (ModelViewMatrixUniform == -1) {
        fprintf(stderr, "Error fetching modelViewMatrixUniform!\n");
        // exit(-1);
    }

    NormalMatrixUniform = glGetUniformLocation(program, "NormalMatrix");

    if (NormalMatrixUniform == -1) {
        fprintf(stderr, "Error fetching normalMatrixUniform!\n");
        // exit(-1);
    }

    TextureMatrixUniform = glGetUniformLocation(program, "TextureMatrix");

    if (TextureMatrixUniform == -1) {
        fprintf(stderr, "Error fetching TextureMatrixUniform!\n");
    }

    ambientLightUniform = glGetUniformLocation(program, "ambientLight");

    if (ambientLightUniform == -1) {
        fprintf(stderr, "Error fetching ambientLightUniform!\n");
        exit(-1);
    }

    light0ColorUniform = glGetUniformLocation(program, "light0Color");

    if (light0ColorUniform == -1) {
        fprintf(stderr, "Error fetching light0ColorUniform!\n");
        // exit(-1);
    }

    light0PositionUniform = glGetUniformLocation(program, "light0Position");

    if (light0PositionUniform == -1) {
        fprintf(stderr, "Error fetching light0PositionUniform!\n");
        // exit(-1);
    }

    materialAmbientUniform = glGetUniformLocation(program, "materialAmbient");

    if (materialAmbientUniform == -1) {
        fprintf(stderr, "Error fetching materialAmbientUniform!\n");
        // exit(-1);
    }


    materialDiffuseUniform = glGetUniformLocation(program, "materialDiffuse");

    if (materialDiffuseUniform == -1) {
        fprintf(stderr, "Error fetching materialDiffuseUniform!\n");
        //exit(-1);
    }

    // labelDiffuseUniform = glGetUniformLocation(program, "labelDiffuse");
    materialSpecularUniform = glGetUniformLocation(program, "materialSpecular");


    materialShininessUniform = glGetUniformLocation(program, "materialShininess");
    texUnitUniform = glGetUniformLocation(program, "texUnit");
    //
    // (9) Tell GL to use our program
    //
    glUseProgram(program);
}

/**
* Surface of Superellipsoid :
* x(u,v) = (cos(v) * |cos(v)|^ 2/m - 1) * (cos(u) * |cos(u)|^ 2/n - 1)
* y(u,v) = (cos(v) * |cos(v)|^ 2/m - 1) * (sin(u) * |sin(u)|^ 2/n - 1)
* z(u,v) = (sin(v) * |sin(v)|^ 2/m - 1) 

where -pie/2 <= v <= pie/2
    -pie <= u <= pie
**/

float absf ( float x ) {

    if ( x < 0.0 )

        return -x;

    return x;

}

GLfloat xValue(GLfloat u,GLfloat v){
    GLfloat Xuv;


    if (cos(v) == 0.0) {
        return 0.0;
    } else if (cos(u) == 0.0) {
        return 0.0;
    } else {
        Xuv = ((float)cos(v) * (float) powf(absf((float)cos(v)),2.0/SMALL_M - 1.0)) *
              ((float) cos(u) * (float) powf( (float) absf(cos(u)), 2.0/SMALL_N - 1.0));

        return Xuv;
    }   

}

GLfloat yValue(GLfloat u,GLfloat v){
    GLfloat Yuv;
    if (cos(v) == 0.0 || sin(u) == 0.0) {
        return 0.0;
    } else {
        Yuv = ((float) cos(v) * (float) powf(absf((float)cos(v)),2.0/SMALL_M - 1.0)) *
              ((float)sin(u) * (float) powf((float)absf(sin(u)), 2.0/SMALL_N - 1.0));

        return Yuv;
    }
}

GLfloat zValue(GLfloat u,GLfloat v){
    GLfloat Zuv;
    if (sin(v) == 0.0) {
        return 0.0;
    } else {
        Zuv = ((float)sin(v) * (float) powf(absf((float)sin(v)),2.0/SMALL_M - 1.0));
        return Zuv;
    }
}

/**
* Surface Normals of Superellipsoid :
* Nx(u,v) = (cos(v) * |cos(v)|^ 1- 2/m) * (cos(u) * |cos(u)|^ 1- 2/n)
* Ny(u,v) = (cos(v) * |cos(v)|^ 1- 2/m) * (sin(u) * |sin(u)|^ 1- 2/n)
* Nz(u,v) = (sin(v) * |sin(v)|^ 1- 2/m) 
*
**/

GLfloat nxValue(GLfloat u, GLfloat v){
    GLfloat NXuv;
    if (cos(v) == 0.0 || cos(u) == 0.0) {
        return 0.0;
    } else {
        NXuv = ((float)cos(v) * (float) powf(absf((float)cos(v)),1.0 - 2.0/SMALL_M)) *
               ((float)cos(u) * (float) powf(absf((float)cos(u)), 1.0 - 2.0/SMALL_N));    

        return NXuv;
    }

}


GLfloat nyValue(GLfloat u, GLfloat v){
    GLfloat NYuv;

    if (cos(v) == 0.0 || sin(u) == 0.0) {
        return 0.0;
    } else {
        NYuv = ((float)cos(v) * (float) powf(absf((float)cos(v)),1.0 - 2.0/SMALL_M)) *
               ((float)sin(u) * (float) powf(absf((float)sin(u)), 1.0 - 2.0/SMALL_N));

        return NYuv;
    }

}

GLfloat nzValue(GLfloat u, GLfloat v){
    GLfloat NZuv;

    if (sin(v) == 0.0) {
        return 0.0;
    } else {
        NZuv = ((float)sin(v) * (float) powf((float)absf(sin(v)),1.0 - 2.0/SMALL_M));
        return NZuv;
    }

}  

void sphericalToCartesian(double radius, double theta, double phi,
                          double *x_Cart, double *y_Cart, double *z_Cart) {
    *x_Cart = radius*cos(theta)*sin(phi);
    *y_Cart = radius*sin(theta)*sin(phi);
    *z_Cart = radius*cos(phi);
}

void ellipsoidWireFrame(){

    static GLuint vertBuffer;
    static GLuint normalsBuffer;
    static GLuint texCoordBuffer;
    static GLuint indexBuffer;
    static GLboolean first = GL_TRUE;
    GLfloat u,v,distanceX,distanceY,distanceZ;
    GLfloat diff[BIG_N+1][BIG_M+1];
    GLfloat sigmaDiK,sigmaDiK2;

    int m,n,i,j,k;
    int cnt=0;

    if (first || changeFlag) {

        for (n=0; n <= BIG_N; n++) {
            v = -M_PI/2.0 + (float) n/ (float) BIG_N * M_PI;

            for (m=0; m <= BIG_M; m++) {
                u = -M_PI + (float) m/ (float) BIG_M * 2.0 * M_PI;

                verticesArray[cnt] = xValue(u,v);
                normalsArray[cnt] = nxValue(u,v);

                cnt++;

                verticesArray[cnt] = yValue(u,v);
                normalsArray[cnt] = nyValue(u,v);

                cnt++;

                verticesArray[cnt] = zValue(u,v);
                normalsArray[cnt] = nzValue(u,v);

                cnt++;                      

            }


        }

        for (i=0;i<=BIG_N;i++) {

            diff[i][0] = 0.0;
            for (j=1;j<=BIG_M;j++) {

                distanceX = verticesArray[3 * (i * (BIG_M + 1) + j)+0] - verticesArray[3 * (i * (BIG_M +1) + j-1) + 0];

                distanceY = verticesArray[3 * (i * (BIG_M + 1) + j)+1] - verticesArray[3 * (i * (BIG_M +1) + j-1) + 1];

                distanceZ = verticesArray[3 * (i * (BIG_M + 1) + j)+2] - verticesArray[3 * (i * (BIG_M +1) + j-1) + 2];

                diff[i][j] = sqrt(distanceX*distanceX +distanceY*distanceY + distanceZ*distanceZ);
            }
        }

        cnt = 0;
        sigmaDiK = 0.0;
        sigmaDiK2 = 0.0;
        for (i=0; i <= BIG_N; i++) {
            for (j=0; j <= BIG_M ; j++) {
                if (i==0 || i==BIG_N) {
                    texCoordArray[cnt][0]=0;
                    cnt++;
                    continue;
                }

                if (j==0) {
                    texCoordArray[cnt][0] = 0 ;
                    cnt++;
                    continue;
                }
                sigmaDiK = 0.0;
                sigmaDiK2 = 0.0;
                for (k=1; k <= j ; k++) {
                    sigmaDiK += diff[i][k];
                }
                for (k=1; k <= BIG_M; k++) {
                    sigmaDiK2 += diff[i][k];
                } 
                texCoordArray[cnt][0] = (float) sigmaDiK/sigmaDiK2;
                cnt++;
            }
        }
        cnt = 0;
        for (i=0; i <= BIG_N; i++) {
            for (j=0; j <= BIG_M ; j++) {
                if (i==0) {
                    texCoordArray[cnt][0]=texCoordArray[cnt+BIG_M+1][0];
                    cnt++;
                    continue;
                }
                if (i==BIG_N) {
                    texCoordArray[cnt][0]=texCoordArray[cnt-(BIG_M+1)][0];
                    cnt++;
                    continue;
                }
                cnt++;
            }
        }


        for (j=0; j <= BIG_M; j++) {
            diff[0][j] = 0.0;
            for (i=1; i<=BIG_N;i++) {

                distanceX = verticesArray[3* (i * (BIG_M + 1) + j)+0] - verticesArray[3 * ((i-1) * (BIG_M +1) + j) + 0];

                distanceY = verticesArray[3* (i * (BIG_M + 1) + j)+1] - verticesArray[3 * ((i-1) * (BIG_M +1) + j) + 1];

                distanceZ = verticesArray[3* (i * (BIG_M + 1) + j)+2] - verticesArray[3 * ((i-1) * (BIG_M +1) + j) + 2];

                diff[i][j] = sqrt(distanceX*distanceX +distanceY*distanceY + distanceZ*distanceZ);

            }
        } 

        cnt = 0;
        sigmaDiK = 0.0;
        sigmaDiK2 = 0.0;
        for (i=0; i <= BIG_N; i++) {
            for (j=0; j <= BIG_M ; j++) {
                if (i==0) {
                    texCoordArray[cnt][1]=0;
                    cnt++;
                    continue;
                }

                if (j==0 || j== BIG_M) {
                    texCoordArray[cnt][1] = 0 ;
                    cnt++;
                    continue;
                }
                sigmaDiK = 0.0;
                sigmaDiK2 = 0.0;
                for (k=1; k <= i ; k++) {
                    sigmaDiK += diff[k][j];
                }
                for (k=1; k <= BIG_N; k++) {
                    sigmaDiK2 += diff[k][j];
                }
                texCoordArray[cnt][1] = (float) sigmaDiK/sigmaDiK2;
                cnt++;
            }
        }

        cnt = 0;
        for (i=0; i <= BIG_N; i++) {
            for (j=0; j <= BIG_M ; j++) {

                if (j==0) {
                    texCoordArray[cnt][1] = texCoordArray[cnt +1][1];
                    cnt++;
                    continue;
                }
                if (j==BIG_M) {
                    texCoordArray[cnt][1] = texCoordArray[cnt -1][1];
                    cnt++;
                    continue;
                }
                cnt++;
            }
        }

        /**
        * For uniformly distributed values:
        cnt = 0;
        for(i=0; i <= BIG_N ; i++){
            for(j=0; j<= BIG_M ; j++){
            
            texCoordArray[cnt][0] = (float) j/BIG_M;
            texCoordArray[cnt][1] = (float) i/BIG_N;
            cnt++;
            
            }
        } */


        glGenBuffers(1, &vertBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verticesArray),
                     verticesArray, GL_STATIC_DRAW);

        glGenBuffers(1, &normalsBuffer);
        glBindBuffer(GL_ARRAY_BUFFER,normalsBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(normalsArray),
                     normalsArray, GL_STATIC_DRAW);

        glGenBuffers(1, &texCoordBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(texCoordArray),
                     texCoordArray, GL_STATIC_DRAW);


        if (first) {
            createMeshStripIndices();
            glGenBuffers(1, &indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tubeStrips),
                         tubeStrips, GL_STATIC_DRAW);
        }
        first = GL_FALSE;
        changeFlag = GL_FALSE;
    }

    loadUniforms();

    glEnableVertexAttribArray(vertexPositionAttr);
    glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
    glVertexAttribPointer(vertexPositionAttr,3, GL_FLOAT,
                          GL_FALSE, 0, (GLvoid*) 0);

    glEnableVertexAttribArray(vertexNormalAttr);
    glBindBuffer(GL_ARRAY_BUFFER, normalsBuffer);   
    glVertexAttribPointer(vertexNormalAttr, 3, GL_FLOAT,
                          GL_FALSE, 0, (GLvoid*) 0);

    glEnableVertexAttribArray(vertexTexCoordAttr);
    glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);  
    glVertexAttribPointer(vertexTexCoordAttr, 2, GL_FLOAT,
                          GL_FALSE, 0, (GLvoid*) 0);


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indexBuffer);
    glDrawElements(GL_QUAD_STRIP, TOTAL_INDICES,
                   GL_UNSIGNED_SHORT, (GLvoid*) 0);


}


void createMeshStripIndices(void){
    int n=0;
    int i,j;
    for (i=0; i< BIG_N; i++) {
        for (j=0; j<= BIG_M; j++) {
            tubeStrips[n] = VERTEX_INDEX(i,j);
            n++;
            tubeStrips[n] = VERTEX_INDEX((i+1) % (BIG_N+1),j);
            n++;
        }
        tubeStrips[n] = tubeStrips[n-1];
        n++;
        tubeStrips[n] = tubeStrips[n-2];
        n++;
        tubeStrips[n] = VERTEX_INDEX(i,j);
        n++;
        tubeStrips[n] = VERTEX_INDEX(i,j);
        n++;
    }

    while (n!= TOTAL_INDICES-1) {
        n--;
    }

}


void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glLineWidth(2.0);
    matrixPush(ModelView);
    matrixRotate(ModelView,center,0,0,1);
    ellipsoidWireFrame();
    matrixPop(ModelView);
    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
#define ESC 27
    if (key == ESC) exit(0);
}

void mouse(int button, int state, int x, int y) {
    mouseX = x;
    mouseY = y;
}

#define RADIANS_PER_PIXEL M_PI/(2*90.0)
#define EPSILON 0.00001
void mouseMotion(int x, int y) {

    int dx = x - mouseX;
    int dy = y - mouseY;
    theta -= dx * RADIANS_PER_PIXEL;
    phi += dy * RADIANS_PER_PIXEL;
    if (phi >= M_PI)
        phi = M_PI - EPSILON;
    else if (phi <= 0.0)
        phi = EPSILON;

    sphericalToCartesian(radius,theta,phi,&eyeX,&eyeY,&eyeZ);
    eyeX += centerX;
    eyeY += centerY;
    eyeZ += centerZ;
    setCamera();
    mouseX = x;
    mouseY = y;
    glutPostRedisplay();
}


void idle(){
    GLfloat seconds = glutGet(GLUT_ELAPSED_TIME)/1000.0;
    GLfloat rotateSpeed = 360.0/40;
    center = rotateSpeed*seconds;
    glutPostRedisplay();

}

void initValues(){
    SMALL_M = 2;
    SMALL_N = 2;
    changeFlag = GL_FALSE;
}

void setCamera() {

    sphericalToCartesian(radius, theta, phi,
                         &eyeX, &eyeY, &eyeZ);
    eyeX += centerX; eyeY += centerY; eyeZ += centerZ;

    matrixIdentity(ModelView);
    matrixLookat(ModelView, eyeX,    eyeY,    eyeZ,
                 centerX, centerY, centerZ,
                 upX,     upY,     upZ);
}

static void displayMenu(int value){

    switch (value) {
    case 1:
        SMALL_M++;
        changeFlag = GL_TRUE;
        break;
    case 2:
        SMALL_M--;
        changeFlag = GL_TRUE;
        break;
    case 3:
        SMALL_N++;
        changeFlag = GL_TRUE;
        break;
    case 4:
        SMALL_N--;
        changeFlag = GL_TRUE;
        break;
    case 5:
        exit(0);
        break;
    }

}

void specialKeyboard(int key,int x,int y)
{
    switch (key) {
    case GLUT_KEY_LEFT:
        SMALL_M++;
        changeFlag = GL_TRUE;
        break;
    case GLUT_KEY_RIGHT:
        SMALL_M--;
        changeFlag = GL_TRUE;
        break;
    case GLUT_KEY_UP:
        SMALL_N++;
        changeFlag = GL_TRUE;
        break;
    case GLUT_KEY_DOWN:
        SMALL_N--;
        changeFlag = GL_TRUE;
        break;
    }
}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800,800);
    glutInitWindowPosition(10,10);
    glutCreateWindow("Super Ellipsoid");

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutIdleFunc(idle);

    glutCreateMenu(displayMenu);
    glutAddMenuEntry("Increase m",1);
    glutAddMenuEntry("Decrease m",2);
    glutAddMenuEntry("Increase n",3);
    glutAddMenuEntry("Decrease n",4);
    glutAddMenuEntry("Exit",5);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    installShaders();

    loadTexture();
    glEnable(GL_TEXTURE_2D);

    setCamera();

    matrixIdentity(Projection);
    matrixPerspective(Projection,
                      40, 1.0, (GLfloat) 1, 750);
    matrixIdentity(TextureMatrix);

    initValues();
    glClearColor(0.0,0.0,0.0,1.0);

    glutMainLoop();
    return 0;
}
