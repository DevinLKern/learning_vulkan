#include <engine/frontend/camera.h>

void SetPerspectiveOrthographicMatrix(Mat4f projection_matrix[static 1], const SetOrthographicsPerspectiveInfo info[static 1])
{
    *projection_matrix          = Mat4f_Identity();
    projection_matrix->data[0]  = 2.0f / (info->right - info->left);
    projection_matrix->data[5]  = 2.0f / (info->bottom - info->top);
    projection_matrix->data[10] = 1.0f / (info->far - info->near);
    projection_matrix->data[12] = -(info->right + info->left) / (info->right - info->left);
    projection_matrix->data[13] = -(info->bottom + info->top) / (info->bottom - info->top);
    projection_matrix->data[14] = -info->near / (info->far - info->near);
}

void SetPerspectiveProjectionMatrix(Mat4f projection_matrix[static 1], const SetPerspectiveProjectionInfo info[static 1])
{
    const float tan_h           = (float)Tan(info->fov_y / 2.0f);
    *projection_matrix          = Mat4f_Identity();
    projection_matrix->data[0]  = 1.0f / (info->aspect_ratio * tan_h);
    projection_matrix->data[5]  = 1.0f / tan_h;
    projection_matrix->data[10] = info->far / (info->far - info->near);
    projection_matrix->data[11] = 1.0f;
    projection_matrix->data[14] = -(info->far * info->near) / (info->far - info->near);
}

/**
 *  0  1  2  3
 *  4  5  6  7
 *  8  9 10 11
 * 12 13 14 15
 */