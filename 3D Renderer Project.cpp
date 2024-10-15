#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cmath>  // For INFINITY and sqrt

// Define basic structures
struct Point {
    float x, y, z;
};

struct Vector {
    float x, y, z;
};

struct Normal {
    float x, y, z;
};

struct Color {
    float r, g, b;
};

struct Ray {
    Point origin;
    Vector direction;
};

struct Object {
    Color color;
    Point center;  // Center of the object (assuming spheres for simplicity)
    float radius;  // Radius of the sphere (if a sphere object)
};

// Function prototypes
void computePrimRay(int i, int j, Ray* primRay);
bool Intersect(const Object& object, const Ray& ray, Point* pHit, Normal* nHit);
float Distance(const Point& p1, const Point& p2);

// Scene setup (global or passed in context)
std::vector<Object> objects;
Point eyePosition = {0, 0, 0};        // Camera position (x, y, z)
Point lightPosition = {10, 10, 10};   // Light source position (x, y, z)
float lightBrightness = 1.0f;
int imageWidth = 800;                 // Width of the image
int imageHeight = 600;                // Height of the image

// Array to store pixel colors (replace with your rendering buffer)
Color pixels[800][600];

// Main rendering loop
int main() {
    // Sample object (Sphere) added to the scene for testing
    Object sphere = {{1.0f, 0.0f, 0.0f}, {0, 0, -5}, 1.0f};  // Red sphere at (0, 0, -5) with radius 1
    objects.push_back(sphere);

    // Ray tracing logic
    for (int j = 0; j < imageHeight; ++j) {
        for (int i = 0; i < imageWidth; ++i) {
            // 1. Determine the direction of the primary ray
            Ray primRay;
            computePrimRay(i, j, &primRay);

            // 2. Initiate a search for intersections within the scene
            Point pHit;
            Normal nHit;
            float minDist = INFINITY;  // Set minimum distance to infinity
            Object* closestObject = nullptr;  // Pointer to the closest intersected object

            // Loop through all objects to find the closest intersection
            for (int k = 0; k < objects.size(); ++k) {
                if (Intersect(objects[k], primRay, &pHit, &nHit)) {
                    float distance = Distance(eyePosition, pHit);  // Calculate distance to the object
                    if (distance < minDist) {
                        closestObject = &objects[k];  // Update the closest object
                        minDist = distance;  // Update minimum distance
                    }
                }
            }

            // Variable to store if the point is in shadow
            bool isInShadow = false;

            // 3. If an intersection is found, check for shadows
            if (closestObject != nullptr) {
                // Cast a shadow ray from the intersection point toward the light
                Ray shadowRay;
                shadowRay.origin = pHit;
                shadowRay.direction = { lightPosition.x - pHit.x, lightPosition.y - pHit.y, lightPosition.z - pHit.z };

                // Check if any object blocks the light
                for (int l = 0; l < objects.size(); ++l) {
                    Point tempHit;
                    Normal tempNormal;
                    if (Intersect(objects[l], shadowRay, &tempHit, &tempNormal)) {
                        isInShadow = true;  // Point is in shadow
                        break;  // No need to check further if we found a blocker
                    }
                }
            }

            // 4. Assign the pixel color based on shadow status
            if (!isInShadow && closestObject != nullptr) {
                // Pixel is illuminated, set its color based on the object's color and light brightness
                pixels[i][j].r = closestObject->color.r * lightBrightness;
                pixels[i][j].g = closestObject->color.g * lightBrightness;
                pixels[i][j].b = closestObject->color.b * lightBrightness;
            } else {
                // Pixel is in shadow, set to black
                pixels[i][j] = {0, 0, 0};  // Black
            }
        }
    }

    // --- SFML Window creation to display the image ---
    sf::RenderWindow window(sf::VideoMode(imageWidth, imageHeight), "Ray Traced Image");

    // Create an SFML image
    sf::Image image;
    image.create(imageWidth, imageHeight);

    // Copy the pixel data into the SFML image
    for (int j = 0; j < imageHeight; ++j) {
        for (int i = 0; i < imageWidth; ++i) {
            sf::Color color(
                static_cast<sf::Uint8>(pixels[i][j].r * 255),
                static_cast<sf::Uint8>(pixels[i][j].g * 255),
                static_cast<sf::Uint8>(pixels[i][j].b * 255)
            );
            image.setPixel(i, j, color);
        }
    }

    // Create a texture and sprite to display the image
    sf::Texture texture;
    texture.loadFromImage(image);
    sf::Sprite sprite(texture);

    // Main loop to keep the window open
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(sprite);  // Draw the sprite containing the ray-traced image
        window.display();
    }

    return 0;
}

// Compute primary ray direction based on pixel coordinates
void computePrimRay(int i, int j, Ray* primRay) {
    primRay->origin = eyePosition;  // The ray originates from the camera
    primRay->direction = {
        (i - imageWidth / 2.0f) / imageWidth, 
        (j - imageHeight / 2.0f) / imageHeight, 
        -1
    };  // A basic direction
}

// Ray-sphere intersection logic
bool Intersect(const Object& object, const Ray& ray, Point* pHit, Normal* nHit) {
    // Ray-sphere intersection (assuming object is a sphere)
    Vector oc = {ray.origin.x - object.center.x, ray.origin.y - object.center.y, ray.origin.z - object.center.z};
    float a = ray.direction.x * ray.direction.x + ray.direction.y * ray.direction.y + ray.direction.z * ray.direction.z;
    float b = 2.0f * (oc.x * ray.direction.x + oc.y * ray.direction.y + oc.z * ray.direction.z);
    float c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - object.radius * object.radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return false;  // No intersection
    } else {
        float t = (-b - sqrt(discriminant)) / (2.0f * a);  // Find the nearest intersection point
        if (t > 0) {
            pHit->x = ray.origin.x + t * ray.direction.x;
            pHit->y = ray.origin.y + t * ray.direction.y;
            pHit->z = ray.origin.z + t * ray.direction.z;

            // Calculate normal at the intersection point
            nHit->x = (pHit->x - object.center.x) / object.radius;
            nHit->y = (pHit->y - object.center.y) / object.radius;
            nHit->z = (pHit->z - object.center.z) / object.radius;

            return true;  // Intersection found
        }
    }
    return false;
}

// Compute the Euclidean distance between two points
float Distance(const Point& p1, const Point& p2) {
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2) + pow(p2.z - p1.z, 2));
}
