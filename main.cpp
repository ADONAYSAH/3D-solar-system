#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <vector>

const int WIDTH = 1280;
const int HEIGHT = 720;
float cameraPos[3] = {0, 0, 50};  // Camera position (x,y,z)
float cameraFront[3] = {0, 0, -1}; //forward and backward movement
float cameraUp[3] = {0, 1, 0}; //upward and downward movement
float cameraSpeed = 0.5f;

int lastX = WIDTH / 2, lastY = HEIGHT / 2;  //last mouse position
bool firstMouse = true;
float yaw = -90.0f, pitch = 0.0f;  //camera rotation angles
// Planet selection and UI
int selectedPlanet = -1; //currently selected planet
bool showDescription = false; // to show planet info or not
float descriptionTimer = 0.0f; // timer for hiding description automatically

float sunRotation = 0.0f;  // Sun's rotation angle
float planetAngles[9] = {0};
float moonAngles[9][20] = {0}; // 20 moon max for 1 planet
float speedFactor = 0.25f;
bool paused = false;
bool welcomeScreen = true;

#define NUM_STARS 2000
float stars[NUM_STARS][3];  //// Star positions (x,y,z)


// Texture handling
GLuint planetTextures[9]; // texture for each planet (including Sun)
GLuint saturnRingTexture;
GLuint moonTextures[10]; // Textures for different moon types
GLuint welcomeTexture;
GLuint asteroidTexture;

// Asteroid structure and container
struct Asteroid {
    float distance;     // Distance from sun
    float angle;
    float speed;
    float size;
    float rotation;
    float rotationSpeed;
    float tilt;           // 	Longer days in summer, shorter in winter
    float orbitTilt;      // usually 0°
};

const int NUM_ASTEROIDS = 500;
std::vector<Asteroid> asteroids; //container for asteroid
// BMP loader
struct imageFile {
    int width;
    int height;
    unsigned char *data;  // pixel data
};
// load image file
imageFile *getBMP(std::string fileName) {
    FILE *file = fopen(fileName.c_str(), "rb");
    if (!file) {
        printf("Error: Unable to open file %s\n", fileName.c_str());
        return NULL;
    }
// Check BMP signature
    unsigned short signature;
    fread(&signature, sizeof(unsigned short), 1, file);
    if (signature != 0x4D42) {
        fclose(file);
        printf("Error: Not a BMP file\n");
        return NULL;
    }
// read dimension
    imageFile *image = new imageFile;
    fseek(file, 18, SEEK_SET);
    fread(&image->width, sizeof(int), 1, file);
    fread(&image->height, sizeof(int), 1, file);
    fseek(file, 2, SEEK_CUR);
    unsigned short bitsPerPixel;
    fread(&bitsPerPixel, sizeof(unsigned short), 1, file);
    if (bitsPerPixel != 24 && bitsPerPixel != 32) {
        fclose(file);
        printf("Error: Only 24-bit and 32-bit BMP files are supported\n");
        delete image;
        return NULL;
    }

    fseek(file, 54, SEEK_SET);
    int size = image->width * image->height * (bitsPerPixel / 8);
    image->data = new unsigned char[size];
    fread(image->data, sizeof(unsigned char), size, file);
    fclose(file);

    for (int i = 0; i < size; i += bitsPerPixel / 8) {
        unsigned char temp = image->data[i];
        image->data[i] = image->data[i + 2];
        image->data[i + 2] = temp;
    }

    return image;
}

void loadTextures() {
    const char* textureFiles[9] = {
        "Sun.bmp",
        "Mercury.bmp",
        "Venus.bmp",
        "Earth.bmp",
        "Mars.bmp",
        "Jupiter.bmp",
        "Saturn.bmp",
        "Uranus.bmp",
        "Neptune.bmp"
    };

    glGenTextures(9, planetTextures);

    for (int i = 0; i < 9; i++) {
        imageFile* image = getBMP(textureFiles[i]);
        if (!image) {
            printf("Failed to load texture: %s\n", textureFiles[i]);
            continue;
        }

        glBindTexture(GL_TEXTURE_2D, planetTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, image->data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        delete[] image->data;
        delete image;
    }

    // Load Saturn's ring texture
    imageFile* ringImage = getBMP("SaturnRing.bmp");
    if (ringImage) {
        glGenTextures(1, &saturnRingTexture);
        glBindTexture(GL_TEXTURE_2D, saturnRingTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ringImage->width, ringImage->height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, ringImage->data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        delete[] ringImage->data;
        delete ringImage;
    } else {
        printf("Failed to load Saturn ring texture\n");
    }

    // Load Moon textures (10 different types)
    const char* moonTextureFiles[10] = {
        "Moon.bmp",       // Earth's Moon
        "Phobos.bmp",     // Mars' moon
        "Deimos.bmp",     // Mars' moon
        "Io.bmp",         // Jupiter's moon
        "Europa.bmp",     // Jupiter's moon
        "Ganymede.bmp",   // Jupiter's moon
        "Callisto.bmp",   // Jupiter's moon
        "Titan.bmp",      // Saturn's moon
        "Triton.bmp",     // Neptune's moon
        "Miranda.bmp"     // Uranus' moon
    };

    glGenTextures(10, moonTextures);

    for (int i = 0; i < 10; i++) {
        imageFile* moonImage = getBMP(moonTextureFiles[i]);
        if (!moonImage) {
            printf("Failed to load moon texture: %s\n", moonTextureFiles[i]);
            continue;
        }

        glBindTexture(GL_TEXTURE_2D, moonTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, moonImage->width, moonImage->height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, moonImage->data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        delete[] moonImage->data;
        delete moonImage;
    }

    // Load Welcome texture
    imageFile* welcomeImage = getBMP("Welcome.bmp");
    if (welcomeImage) {
        glGenTextures(1, &welcomeTexture);
        glBindTexture(GL_TEXTURE_2D, welcomeTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, welcomeImage->width, welcomeImage->height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, welcomeImage->data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        delete[] welcomeImage->data;
        delete welcomeImage;
    } else {
        printf("Failed to load Welcome texture\n");
    }

    // Load Asteroid texture
    imageFile* asteroidImage = getBMP("Asteroid.bmp");
    if (asteroidImage) {
        glGenTextures(1, &asteroidTexture);
        glBindTexture(GL_TEXTURE_2D, asteroidTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, asteroidImage->width, asteroidImage->height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, asteroidImage->data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        delete[] asteroidImage->data;
        delete asteroidImage;
    } else {
        printf("Failed to load Asteroid texture\n");
    }
}
// Normalize a 3D vector move direction but not magnitude
void normalize(float* v) {
    float len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]); // calculate magnitude from the sun
    if (len > 0) {//to make it to a unit vector (no change in magnitude)
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}
// takes 2 vectors and computes a third on perpendicular to both(used for used to strafe left/right for camera )
void crossProduct(float* a, float* b, float* result) {
    result[0] = a[1]*b[2] - a[2]*b[1];
    result[1] = a[2]*b[0] - a[0]*b[2];
    result[2] = a[0]*b[1] - a[1]*b[0];
}
//draws 2d text
void drawText(float x, float y, const char* text) {
    glMatrixMode(GL_PROJECTION); // switch projection matrix
    glPushMatrix(); // push current matrix to stack
    glLoadIdentity(); //reset projection matrix
    gluOrtho2D(0, WIDTH, 0, HEIGHT); // setup 2d projection
    // model view matrix(handle positioning of objects)
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glRasterPos2f(x, y);
    // loops and draws each character
    for (const char* c = text; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
// draws a simple sphere for stars
void drawSphere(float r, int slices = 30, int stacks = 30) {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluSphere(quad, r, slices, stacks);
    gluDeleteQuadric(quad);
}
// Draw a sphere with texture for planets
void drawTexturedSphere(float radius, int textureID) {
    glBindTexture(GL_TEXTURE_2D, textureID); // tells to follow texture operations

    GLUquadric* quad = gluNewQuadric(); //creates quadratic object
    gluQuadricTexture(quad, GL_TRUE); // allows for texture coordination
    gluQuadricNormals(quad, GLU_SMOOTH); // smooth surface
    gluSphere(quad, radius, 30, 30); // draws the sphere of the quadratic object
    gluDeleteQuadric(quad); // delete quadratic object
}
// draw textured ring for saturn
void drawTexturedRing(float r1, float r2, GLuint texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
    GLUquadric* q = gluNewQuadric();
    gluQuadricTexture(q, GL_TRUE);
    gluQuadricNormals(q, GLU_SMOOTH);
    gluDisk(q, r1, r2, 50, 1);// draws the ring
    gluDeleteQuadric(q);
}
// draws a planets orbit based on radius
void drawOrbit(float r) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; ++i) {
        float theta = i * 3.14159f / 180.0f;
        glVertex3f(r * cos(theta), 0, r * sin(theta));
    }
    glEnd();
}
// starfield
void initStars() {
    srand(time(NULL));
    for (int i = 0; i < NUM_STARS; i++) {// // Random spherical coordinates
        float theta = 2.0f * M_PI * (rand() / (float)RAND_MAX);
        float phi = acos(1.0f - 2.0f * (rand() / (float)RAND_MAX));
        float r = 100.0f + 900.0f * (rand() / (float)RAND_MAX);

        stars[i][0] = r * sin(phi) * cos(theta);
        stars[i][1] = r * sin(phi) * sin(theta);
        stars[i][2] = r * cos(phi);
    }
}
// Initialize asteroid belt
void initAsteroids() {
    srand(time(NULL));// random nmber of asteroids by using time
    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        Asteroid a;
        a.distance = 12.0f + (rand() / (float)RAND_MAX) * 4.0f;// between mars and Jupiter between 12.0 and 16.0
        a.angle = rand() / (float)RAND_MAX * 360.0f; // angle from the sun
        a.speed = 0.1f + (rand() / (float)RAND_MAX) * 0.4f; // orbital speed
        a.size = 0.02f + (rand() / (float)RAND_MAX) * 0.08f; // size between  0.02 to 0.10 units
        a.rotation = rand() / (float)RAND_MAX * 360.0f; // spinning in own axis
        a.rotationSpeed = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;// rotation speen
        a.tilt = rand() / (float)RAND_MAX * 360.0f; // random tilt
        a.orbitTilt = (rand() / (float)RAND_MAX) * 15.0f - 7.5f; // small tilt

        asteroids.push_back(a);
    }
}

void drawStars() {
    glDisable(GL_LIGHTING); // so stars are not affected by any scene lights
    glColor3f(1.0f, 1.0f, 1.0f); // white
    glDisable(GL_TEXTURE_2D); // no textures needed

    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_STARS; i++) { //loops throug with randomm positions
        float brightness = 0.5f + 0.5f * (rand() / (float)RAND_MAX); // rand om brightness
        glColor3f(brightness, brightness, brightness);// set the brightness
        glVertex3fv(stars[i]); // draws a point at random coordinate
    }
    glEnd();

    glEnable(GL_TEXTURE_2D); // re- enables texture and lightnng
    glEnable(GL_LIGHTING);
}

void drawAsteroids() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, asteroidTexture);

    GLfloat matAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f}; // base color in shadow
    GLfloat matDiffuse[] = {0.7f, 0.7f, 0.7f, 1.0f}; //main surface color under direct ligh
    GLfloat matSpecular[] = {0.1f, 0.1f, 0.1f, 1.0f};//shininess (reflection highlight
    GLfloat matShininess[] = {5.0f}; //  controls how concentrated the reflection is

    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);

    for (const Asteroid& a : asteroids) {
        glPushMatrix();
        glRotatef(a.orbitTilt, 0, 0, 1);
        glRotatef(a.angle, 0, 1, 0);
        glTranslatef(a.distance, 0, 0);
        glRotatef(a.tilt, 0, 0, 1);
        glRotatef(a.rotation, 0, 1, 0);
        drawTexturedSphere(a.size, asteroidTexture);
        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);
}

struct Moon {
    const char* name;
    float size;
    float distance;
    float speed;
    float rotationSpeed;
    float axisTilt;
    float orbitTilt;
    int textureIndex;
    bool retrograde;
};

struct Planet {
    const char* name;
    float size; // radius
    float distance;
    float speed;
    float rotationSpeed;
    float axisTilt;
    const char* description;
    const char* composition;
    const char* specialFeatures;
    int moons;
    Moon moonData[20]; // Increased for planets with many moons
    float temperature;
};

Planet planets[9] = {
    {"Sun",
     5.0f, 0.0f, 1.0f, 0.3f, 0.0f,
     "The Sun is a hot sphere of plasma that powers life on Earth. It generates energy through nuclear fusion and has strong magnetic fields that cause sunspots.",
     "Mostly hydrogen (74.9%) and helium (23.8%) with traces of heavier elements.",
     "The core reaches 15 million °C. Sunspots are caused by magnetic activity and appear as dark areas.",
     0, {}, 5500.0f},

    {"Mercury",
     0.4f, 6.0f, 4.8f, 0.1f, 2.11f,
     "Mercury is the smallest planet, with a surface full of craters. It has long days and a very thin atmosphere.",
     "70% metal and 30% silicate, with a large iron core.",
     "Contains water ice at its poles and has an exosphere formed by solar radiation.",
     0, {}, 167.0f},

    {"Venus",
     0.9f, 8.0f, 3.5f, 0.002f, 177.4f,
     "Venus is Earth's sister in size but has a thick CO₂ atmosphere and extreme surface pressure and heat.",
     "Rocky mantle, basaltic crust, and possibly a liquid core.",
     "It spins backwards and has many volcanoes hidden by clouds of sulfuric acid.",
     0, {}, 464.0f},

    {"Earth",
     1.0f, 10.0f, 3.0f, 1.0f, 23.44f,
     "Earth is the only known planet with life and liquid water. It has active geology and supports diverse ecosystems.",
     "Iron core, mantle rich in magnesium and iron, and a crust of oxygen, silicon, and aluminum.",
     "Its magnetic field and the Moon help stabilize the climate and protect life.",
     1,
     {{"Moon", 0.2f, 1.5f, 2.0f, 0.03f, 6.68f, 5.1f, 0, false}}, // Earth's Moon
     15.0f},

    {"Mars",
     0.5f, 12.0f, 2.4f, 0.97f, 25.19f,
     "Mars is a cold, dusty world with signs of past water. It has giant volcanoes and deep canyons.",
     "Silicate mantle and an iron-nickel-sulfur core.",
     "Home to Phobos and Deimos, and may still hold underground water.",
     2,
     {
        {"Phobos", 0.1f, 1.0f, 3.0f, 0.05f, 0.0f, 1.1f, 1, false},
        {"Deimos", 0.08f, 1.2f, 1.5f, 0.02f, 0.0f, 0.9f, 2, false}
     },
     -63.0f},

    {"Jupiter",
     2.0f, 16.0f, 1.3f, 2.4f, 3.13f,
     "Jupiter is the largest planet with a strong magnetic field and a giant storm called the Great Red Spot.",
     "Dense core with layers of metallic and molecular hydrogen.",
     "Has 79 moons including the Galilean moons, and powerful radiation belts.",
     4, // Showing the 4 Galilean moons
     {
        {"Io", 0.3f, 2.5f, 1.2f, 0.1f, 0.0f, 0.04f, 3, false},
        {"Europa", 0.28f, 3.0f, 0.8f, 0.09f, 0.1f, 0.47f, 4, false},
        {"Ganymede", 0.35f, 3.5f, 0.6f, 0.08f, 0.2f, 0.2f, 5, false},
        {"Callisto", 0.25f, 4.0f, 0.5f, 0.07f, 0.3f, 0.2f, 6, false}
     },
     -145.0f},

    {"Saturn",
     1.7f, 20.0f, 1.0f, 2.3f, 26.73f,
     "Saturn is known for its bright rings and low density. It's made mostly of hydrogen and helium.",
     "Core of rock and metal, surrounded by hydrogen and helium.",
     "Its rings are wide but thin, and it has 82 moons, including Titan.",
     7, // Showing major moons
     {
        {"Mimas", 0.25f, 2.2f, 0.4f, 0.06f, 0.0f, 1.53f, 0, false},
        {"Enceladus", 0.3f, 2.5f, 0.3f, 0.05f, 0.1f, 0.0f, 0, false},
        {"Tethys", 0.4f, 3.0f, 0.2f, 0.04f, 0.2f, 1.86f, 0, false},
        {"Dione", 0.35f, 3.5f, 0.15f, 0.03f, 0.3f, 0.02f, 0, false},
        {"Rhea", 0.45f, 4.0f, 0.1f, 0.02f, 0.4f, 0.35f, 0, false},
        {"Titan", 0.5f, 5.0f, 0.08f, 0.01f, 0.5f, 0.33f, 7, false},
        {"Iapetus", 0.3f, 6.0f, 0.05f, 0.005f, 15.0f, 7.57f, 0, false}
     },
     -178.0f},

    {"Uranus",
     1.4f, 24.0f, 0.7f, 1.4f, 97.77f,
     "Uranus spins on its side, leading to extreme seasons. It's cold and has a faint ring system.",
     "Icy mantle with a rocky core, and an atmosphere of hydrogen and helium.",
     "Has 27 moons and 13 rings, and may have tilted due to a collision.",
     5, // Showing major moons
     {
        {"Miranda", 0.2f, 1.8f, 0.25f, 0.05f, 0.0f, 4.22f, 9, false},
        {"Ariel", 0.15f, 2.0f, 0.2f, 0.04f, 0.1f, 0.31f, 0, false},
        {"Umbriel", 0.18f, 2.2f, 0.15f, 0.03f, 0.2f, 0.36f, 0, false},
        {"Titania", 0.22f, 2.5f, 0.1f, 0.02f, 0.3f, 0.14f, 0, false},
        {"Oberon", 0.2f, 2.8f, 0.08f, 0.01f, 0.4f, 0.1f, 0, false}
     },
     -224.0f},

    {"Neptune",
     1.3f, 28.0f, 0.5f, 1.5f, 28.32f,
     "Neptune is a windy, icy giant discovered through math. It has strong storms and a deep blue color.",
     "Rocky core with icy mantle and a hydrogen-helium atmosphere.",
     "It has 14 moons, including Triton, which orbits backward.",
     1,
     {{"Triton", 0.25f, 2.0f, 0.3f, 0.04f, 157.0f, 130.0f, 8, true}}, // Triton (with retrograde orbit)(rotates in oposite direction)
     -214.0f}
};

void drawMoon(const Moon& moon, float angle) {
    glPushMatrix();// Saves the current transformation matrix

    // Apply orbit tilt
    glRotatef(moon.orbitTilt, 0, 0, 1);// tilts around z axis

    // Handle retrograde motion
    if (moon.retrograde) {// if true orbit backward
        glRotatef(-angle, 0, 1, 0);
    } else {
        glRotatef(angle, 0, 1, 0);
    }

    glTranslatef(moon.distance, 0, 0);//distance away from its planet, along the X-axis
    glRotatef(moon.axisTilt, 0, 0, 1);

    // Moon rotation
    if (moon.retrograde) {// Rotates the moon around its own axis (Y-axis)
        glRotatef(-angle * moon.rotationSpeed, 0, 1, 0);
    } else {
        glRotatef(angle * moon.rotationSpeed, 0, 1, 0);
    }

    // Set material properties color shininess light reflection
    GLfloat matAmbient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat matDiffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat matSpecular[] = {0.3f, 0.3f, 0.3f, 1.0f};
    GLfloat matShininess[] = {10.0f};

    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);

    if (moon.textureIndex >= 0 && moon.textureIndex < 10) {// if texture is assigned draw a textured moon , other wise build a plain one
        drawTexturedSphere(moon.size, moonTextures[moon.textureIndex]);//Only use a texture if the moon's texture index is within a valid range — from 0 to 9.
    } else {
        drawSphere(moon.size);
    }

    glPopMatrix();
}

void drawMoons(int planetIdx) {
    if (planets[planetIdx].moons <= 0) return;

    float angle = planetAngles[planetIdx] * 3.14159f / 180.0f;
    float x = planets[planetIdx].distance * cos(angle);
    float z = planets[planetIdx].distance * sin(angle);

    glPushMatrix();
    glTranslatef(x, 0.0, z);
    glRotatef(planets[planetIdx].axisTilt, 0, 0, 1);

    for (int i = 0; i < planets[planetIdx].moons; i++) {
        drawMoon(planets[planetIdx].moonData[i], moonAngles[planetIdx][i]);
    }

    glPopMatrix();
}

void drawPlanetWithTexture(int planetIdx) {
    float angle = planetAngles[planetIdx] * 3.14159f / 180.0f;
    float x = planets[planetIdx].distance * cos(angle);
    float z = planets[planetIdx].distance * sin(angle);

    glPushMatrix();
    if (planetIdx > 0) { // Only translate planets (not Sun)
        glTranslatef(x, 0.0, z);
    }
    glRotatef(planets[planetIdx].axisTilt, 0, 0, 1);
    glRotatef(planetAngles[planetIdx] * planets[planetIdx].rotationSpeed, 0, 1, 0);

    if (selectedPlanet == planetIdx) {
        glDisable(GL_LIGHTING);
        glColor3f(1, 1, 0);
        drawSphere(planets[planetIdx].size * 1.1f);
        glEnable(GL_LIGHTING);
    }

    // Set material properties
    GLfloat matAmbient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat matDiffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat matSpecular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat matShininess[] = {50.0f};

    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);

    // Special case for Sun - make it emit light
    if (planetIdx == 0) {
        GLfloat emit[] = {0.8f, 0.8f, 0.6f, 1.0f};
        glMaterialfv(GL_FRONT, GL_EMISSION, emit);
    }

    // Draw the textured sphere
    glColor3f(1.0f, 1.0f, 1.0f);
    drawTexturedSphere(planets[planetIdx].size, planetTextures[planetIdx]);

    // Reset emission for non-sun objects
    if (planetIdx == 0) {
        GLfloat noEmit[] = {0, 0, 0, 1};
        glMaterialfv(GL_FRONT, GL_EMISSION, noEmit);
    }

    if (planetIdx == 6) { // Saturn's rings
        glPushMatrix();
        glRotatef(-20.0f, 1, 0, 0);
        drawTexturedRing(planets[planetIdx].size * 1.1f, planets[planetIdx].size * 1.8f, saturnRingTexture);
        glPopMatrix();
    }

    glPopMatrix();
}

void drawWelcomeScreen() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, welcomeTexture);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(WIDTH, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(WIDTH, HEIGHT);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, HEIGHT);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(WIDTH/2 - 100, HEIGHT/4, "Press ENTER to Explore the Solar System");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

void display() {
    if (welcomeScreen) {
        drawWelcomeScreen();
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    gluLookAt(cameraPos[0], cameraPos[1], cameraPos[2],
              cameraPos[0] + cameraFront[0], cameraPos[1] + cameraFront[1], cameraPos[2] + cameraFront[2],
              cameraUp[0], cameraUp[1], cameraUp[2]);

    drawStars();

    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.5f);
    for (int i = 1; i < 9; ++i) drawOrbit(planets[i].distance);
    glEnable(GL_LIGHTING);

    // Draw Sun with texture
    glPushMatrix();
    glRotatef(sunRotation, 0, 1, 0);
    drawPlanetWithTexture(0);
    glPopMatrix();

    for (int i = 1; i < 9; ++i) {
        drawPlanetWithTexture(i);
        drawMoons(i);
    }

    // Draw the asteroid belt between Mars and Jupiter
    drawAsteroids();

    glDisable(GL_LIGHTING);
    if (showDescription && selectedPlanet != -1) {
        glColor3f(1, 1, 1);

        char infoLine1[256];
        sprintf(infoLine1, "%s | Moons: %d | Avg Temp: %.1f°C",
                planets[selectedPlanet].name,
                planets[selectedPlanet].moons,
                planets[selectedPlanet].temperature);
        drawText(30, HEIGHT - 50, infoLine1);

        char infoLine2[256];
        sprintf(infoLine2, "Composition: %s", planets[selectedPlanet].composition);
        drawText(30, HEIGHT - 80, infoLine2);

        const char* desc = planets[selectedPlanet].description;
        int len = strlen(desc);
        int lineLen = 100;
        int numLines = (len / lineLen) + 1;

        for (int i = 0; i < numLines; i++) {
            char line[256];
            strncpy(line, desc + (i * lineLen), lineLen);
            line[lineLen] = '\0';
            drawText(30, HEIGHT - 110 - (i * 25), line);
        }

        const char* features = planets[selectedPlanet].specialFeatures;
        len = strlen(features);
        numLines = (len / lineLen) + 1;

        for (int i = 0; i < numLines; i++) {
            char line[256];
            strncpy(line, features + (i * lineLen), lineLen);
            line[lineLen] = '\0';
            drawText(30, HEIGHT - 110 - (numLines * 25) - 30 - (i * 25), line);
        }

        // Show moon names if planet has moons
        if (planets[selectedPlanet].moons > 0) {
            drawText(30, HEIGHT - 110 - (numLines * 25) - 60, "Major Moons:");
            for (int i = 0; i < planets[selectedPlanet].moons; i++) {
                char moonLine[256];
                sprintf(moonLine, "%d. %s", i+1, planets[selectedPlanet].moonData[i].name);
                drawText(50, HEIGHT - 110 - (numLines * 25) - 85 - (i * 25), moonLine);
            }
        }
    }

    glColor3f(0.8f, 0.8f, 0.8f);
    drawText(30, 60, "1 - Pause | 2 - Slow (0.25x) | 3 - Normal (1x) | 4 - Fast (2x) | 5 - Very Fast (4x)");
    drawText(30, 30, "WASD - Move | Mouse - Look | Click - Info | ESC - Exit");

    char speedStatus[50];
    if (paused) {
        sprintf(speedStatus, "Speed: PAUSED");
    } else {
        sprintf(speedStatus, "Speed: %.2fx", speedFactor);
    }
    drawText(WIDTH - 150, 30, speedStatus);

    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

void update(int val) {
    if (!paused && !welcomeScreen) {
        sunRotation += 0.3f * speedFactor;
        for (int i = 1; i < 9; ++i) {
            planetAngles[i] += planets[i].speed * speedFactor;
            if (planetAngles[i] > 360) planetAngles[i] -= 360;

            // Update moon angles for planets that have moons
            for (int j = 0; j < planets[i].moons; j++) {
                moonAngles[i][j] += planets[i].moonData[j].speed * speedFactor;
                if (moonAngles[i][j] > 360) moonAngles[i][j] -= 360;
            }
        }

        // Update asteroid positions
        for (Asteroid& a : asteroids) {
            a.angle += a.speed * speedFactor;
            if (a.angle > 360) a.angle -= 360;
            a.rotation += a.rotationSpeed * speedFactor;
            if (a.rotation > 360) a.rotation -= 360;
        }
    }

    if (showDescription) {
        descriptionTimer += 0.016f;
        if (descriptionTimer > 10.0f) {
            showDescription = false;
            selectedPlanet = -1;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w / h, 1.0, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

void mouseMotion(int x, int y) {
    if (welcomeScreen) return;

    if (firstMouse) {
        lastX = x; lastY = y; firstMouse = false;
    }

    float xOffset = x - lastX;
    float yOffset = lastY - y;
    lastX = x; lastY = y;

    float sensitivity = 0.1f;
    yaw += xOffset * sensitivity;
    pitch += yOffset * sensitivity;
    if (pitch > 89) pitch = 89;
    if (pitch < -89) pitch = -89;

    float radYaw = yaw * 3.14159f / 180;
    float radPitch = pitch * 3.14159f / 180;
    cameraFront[0] = cos(radYaw) * cos(radPitch);
    cameraFront[1] = sin(radPitch);
    cameraFront[2] = sin(radYaw) * cos(radPitch);
    normalize(cameraFront);
}

void keyboard(unsigned char key, int, int) {
    if (welcomeScreen) {
        if (key == 13) { // Enter key
            welcomeScreen = false;
            glutPostRedisplay();
        }
        return;
    }

    float right[3];
    crossProduct(cameraFront, cameraUp, right);
    normalize(right);

    switch(key) {
        case 'w':
            for (int i = 0; i < 3; i++) cameraPos[i] += cameraFront[i] * cameraSpeed;
            break;
        case 's':
            for (int i = 0; i < 3; i++) cameraPos[i] -= cameraFront[i] * cameraSpeed;
            break;
        case 'a':
            for (int i = 0; i < 3; i++) cameraPos[i] -= right[i] * cameraSpeed;
            break;
        case 'd':
            for (int i = 0; i < 3; i++) cameraPos[i] += right[i] * cameraSpeed;
            break;
        case '1':
            paused = true;
            break;
        case '2':
            paused = false;
            speedFactor = 0.25f;
            break;
        case '3':
            paused = false;
            speedFactor = 1.0f;
            break;
        case '4':
            paused = false;
            speedFactor = 2.0f;
            break;
        case '5':
            paused = false;
            speedFactor = 4.0f;
            break;
        case 27: // ESC key
            exit(0);
            break;
    }
}

void mouseClick(int button, int state, int x, int y) {
    if (welcomeScreen) return;

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        GLuint buffer[64];
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        glSelectBuffer(64, buffer);
        glRenderMode(GL_SELECT);
        glInitNames();

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluPickMatrix(x, viewport[3] - y, 5, 5, viewport);
        gluPerspective(45.0, (float)WIDTH / HEIGHT, 1.0, 200.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        gluLookAt(cameraPos[0], cameraPos[1], cameraPos[2],
                  cameraPos[0] + cameraFront[0], cameraPos[1] + cameraFront[1], cameraPos[2] + cameraFront[2],
                  cameraUp[0], cameraUp[1], cameraUp[2]);

        for (int i = 0; i < 9; ++i) {
            glPushName(i);
            glPushMatrix();
            if (i > 0) {
                float a = planetAngles[i] * 3.14159f / 180.0f;
                float x = planets[i].distance * cos(a);
                float z = planets[i].distance * sin(a);
                glTranslatef(x, 0.0, z);
            }
            drawSphere(planets[i].size);
            glPopMatrix();
            glPopName();
        }

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        int hits = glRenderMode(GL_RENDER);
        if (hits > 0) {
            selectedPlanet = buffer[3];
            showDescription = true;
            descriptionTimer = 0.0f;
        }
    }
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glClearColor(0.0, 0.0, 0.05, 1.0);
    glPointSize(2.0f);

    // Enable texture mapping
    glEnable(GL_TEXTURE_2D);

    GLfloat lightPos[] = {0, 0, 0, 1};
    GLfloat lightColor[] = {1.0f, 0.9f, 0.8f, 1.0f};
    GLfloat ambient[] = {0.1f, 0.1f, 0.1f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    initStars();
    initAsteroids();
    loadTextures();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("3D Solar System Simulation");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(16, update, 0);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseClick);
    glutPassiveMotionFunc(mouseMotion);

    glutMainLoop();
    return 0;
}
