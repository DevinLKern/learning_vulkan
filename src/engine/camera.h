#ifndef CAMERA_H
#define CAMERA_H

#include <utility/math.h>

typedef struct Camera
{
    // camera to screen
    Mat4f projection_matrix;
    // world space to camera space
    Mat4f view_matrix;
} Camera;

typedef struct SetOrthographicsPerspectiveInfo
{
    float left;
    float right;
    float top;
    float bottom;
    float near;
    float far;
} SetOrthographicsPerspectiveInfo;
void SetPerspectiveOrthographicMatrix(Mat4f projection_matrix[static 1], const SetOrthographicsPerspectiveInfo info[static 1]);

typedef struct SetPerspectiveProjectionInfo
{
    float fov_y;
    float aspect_ratio;
    float near;
    float far;
} SetPerspectiveProjectionInfo;
void SetPerspectiveProjectionMatrix(Mat4f projection_matrix[static 1], const SetPerspectiveProjectionInfo info[static 1]);

void LookAtCoordinate(const Vec3f location[static 1], const Vec3f target[static 1], const Vec3f up[static 1]);

#endif