#ifndef CAMERA_HPP_
#define CAMERA_HPP_

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <scene_object.hpp>

/// Defines several possible options for camera movement. 
/// Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class FirstPersonCamera : public SceneObject
{
private:
    // default camera values
    // yaw is initialized to -90.0 degrees since a yaw of 0.0 results
    // in a direction vector pointing to the right so we initially rotate a bit to the left.
    const float SPEED = 0.07f;
    const float SENSITIVITY = 0.07f;
    const float ZOOM = 45.0f;
    const float NEAR_PLANE = 0.1;
    const float FAR_PLANE = 100.0;

    // camera Attributes
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    // euler Angles
    float yaw;
    float pitch;
    // camera options
    float nearPlane;
    float farPlane;
    float movementSpeed;
    float mouseSensitivity;
    float zoom;

    float aspectRatio;

public:
    /// constructor with vectors
    FirstPersonCamera(
        float aspRation,
        glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f), 
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
        glm::vec3 f = glm::vec3(0.0f, 0.0f, -1.0f),
        float y = -90.0f,
        float p = 0.0f) 
        : aspectRatio(aspRation)
        , front(f)
        , worldUp(up)
        , yaw(y)
        , pitch(p)
        , movementSpeed(SPEED)
        , mouseSensitivity(SENSITIVITY)
        , zoom(ZOOM)
        , nearPlane(NEAR_PLANE)
        , farPlane(FAR_PLANE)
    {
        position = pos;
        updateCameraVectors();
    }

    /// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void processKeyboard(Camera_Movement direction)
    {
        if (direction == FORWARD)
            position += front * movementSpeed;
        if (direction == BACKWARD)
            position -= front * movementSpeed;
        if (direction == LEFT)
            position -= right * movementSpeed;
        if (direction == RIGHT)
            position += right * movementSpeed;
        if (direction == DOWN)
            position -= up * movementSpeed;
        if (direction == UP)
            position += up * movementSpeed;
    }

    /// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch -= yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    /// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void processMouseScroll(float yoffset)
    {
        zoom -= (float)yoffset;
        if (zoom < 1.0f)
            zoom = 1.0f;
        if (zoom > 45.0f)
            zoom = 45.0f;
    }

    glm::mat4 getProjectionMatrix() const {
        return glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
    }
    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    void setAspectRatio(float a) { aspectRatio = a; }
    void setSpeed(float s) { movementSpeed = s; }

    float near() const { return nearPlane; }
    float far() const { return farPlane; }
    float getZoom() const { return zoom; }
    void resetRotation() {
        pitch = 0.0f;
        yaw = -90.0f;
    }

private:
    /// calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        right = glm::normalize(glm::cross(front, worldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        up = glm::normalize(glm::cross(right, front));
    }
};
#endif