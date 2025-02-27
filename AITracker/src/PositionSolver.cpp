#include "PositionSolver.h"


PositionSolver::PositionSolver(
    int width, 
    int height,
    float prior_pitch, 
    float prior_yaw, 
    float prior_distance, 
    bool complex, 
    float fov,
    float x_scale,
    float y_scale, 
    float z_scale) :

    //TODO: Refactor removing prior_yaw/pitch parameters
    landmark_points_buffer(complex ? NB_CONTOUR_POINTS_COMPLEX: NB_CONTOUR_POINTS_BASE, 1, CV_32FC2),
    rv({ 0, 0, 0 }),
    tv({ 0, 0, 0 })
{
    this->prior_pitch = -1.57;
    this->prior_yaw = -1.57;
    this->prior_distance = prior_distance * -2.;

    this->rv[0] = this->prior_pitch;
    this->rv[1] = this->prior_yaw;
    this->rv[2] = -1.57;
    this->tv[2] = this->prior_distance;

    head3dScale = (cv::Mat_<double>(3, 3) <<
        x_scale, 0.0, 0,
        0.0, y_scale, 0,
        0.0, 0.0, z_scale
    );

    this->complex = complex;

    if (!complex)
    { 
        contour_indices = { 0,1,8,15,16,27,28,29,30,31,32,33,34,35,36,39,42,45 };
        mat3dcontour = (cv::Mat_<double>(NB_CONTOUR_POINTS_BASE, 3) <<
            0.45517698, -0.30089578, 0.76442945,
            0.44899884, -0.16699584, 0.765143,
            0., 0.621079, 0.28729478,
            -0.44899884, -0.16699584, 0.765143,
            -0.45517698, -0.30089578, 0.76442945,
            0., -0.2933326, 0.1375821,
            0., -0.1948287, 0.06915811,
            0., -0.10384402, 0.00915182,
            0., 0., 0.,
            0.08062635, 0.04127607, 0.13416104,
            0.04643935, 0.05767522, 0.10299063,
            0., 0.06875312, 0.09054535,
            -0.04643935, 0.05767522, 0.10299063,
            -0.08062635, 0.04127607, 0.13416104,
            0.31590518, -0.2983375, 0.2851074,
            0.13122973, -0.28444737, 0.23423915,
            -0.13122973, -0.28444737, 0.23423915,
            -0.31590518, -0.2983375, 0.2851074
            );
    }
    else
    {
        contour_indices = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,39,42,45 };

        landmark_points_buffer = cv::Mat(contour_indices.size(), 1, CV_32FC2);

        mat3dcontour = (cv::Mat_<double>(contour_indices.size(), 3) <<
            0.45517698, -0.30089578, 0.76442945,
            0.44899884, -0.16699584, 0.76514298,
            0.43743154, -0.02265548, 0.73926717,
            0.41503343, 0.08894145, 0.74794745,
            0.38912359, 0.23238003, 0.70478839,
            0.3346301, 0.36126539, 0.61558759,
            0.2637251, 0.46000972, 0.49147922,
            0.16241622, 0.55803716, 0.33944517,
            0., 0.62107903, 0.28729478,
            -0.16241622, 0.55803716, 0.33944517,
            -0.2637251, 0.46000972, 0.49147922,
            -0.3346301, 0.36126539, 0.61558759,
            -0.38912359, 0.23238003, 0.70478839,
            -0.41503343, 0.08894145, 0.74794745,
            -0.43743154, -0.02265548, 0.73926717,
            -0.44899884, -0.16699584, 0.76514298,
            0., -0.29333261, 0.13758209,
            0., -0.1948287, 0.06915811,
            0., -0.10384402, 0.00915182,
            0., 0., 0.,
            0.08062635, 0.04127607, 0.13416104,
            0.04643935, 0.05767522, 0.10299063,
            0., 0.06875312, 0.09054535,
            -0.04643935, 0.05767522, 0.10299063,
            -0.08062635, 0.04127607, 0.13416104,
            0.31590518, -0.29833749, 0.2851074,
            0.13122973, -0.28444737, 0.23423915,
            -0.13122973, -0.28444737, 0.23423915,
            -0.31590518, -0.29833749, 0.2851074
            );
    }

    // Taken from 
    // https://github.com/opentrack/opentrack/blob/3cc3ef246ad71c463c8952bcc96984b25d85b516/tracker-aruco/ftnoir_tracker_aruco.cpp#L193
    // Taking into account the camera FOV instead of assuming raw image dims is more clever and
    // will make the solver more camera-agnostic.
    float diag_fov = fov * TO_RAD;

    // Get expressed in sensor-size units

    double fov_w = 2. * atan(tan(diag_fov / 2.) / sqrt(1. + height / (double)width * height / (double)width));
    double fov_h = 2. * atan(tan(diag_fov / 2.) / sqrt(1. + width / (double)height * width / (double)height));

    float i_height = .5 * height / (tan(.5*fov_w));
    float i_width = .5* width / (tan(.5*fov_h));

    /*camera_matrix = (cv::Mat_<double>(3, 3) <<
        height, 0, height / 2,
        0, height, width / 2,
        0, 0, 1
        );*/

    camera_matrix = (cv::Mat_<double>(3, 3) <<
        i_width, 0, height / 2,
        0, i_height, width / 2,
        0, 0, 1
     );

    camera_distortion = (cv::Mat_<double>(4, 1) << 0, 0, 0, 0);

    mat3dcontour =  mat3dcontour * head3dScale;

    if(complex) std::cout << "Using complex solver" << std::endl;
}

void PositionSolver::solve_rotation(FaceData* face_data)
{
    int contour_idx = 0;
    for (int j = 0; j < 2; j++)
    {
        for (long unsigned int i = 0; i < contour_indices.size(); i++)
        {
            contour_idx = contour_indices[i];
            landmark_points_buffer.at<float>(i, j) = (int)face_data->landmark_coords[2 * contour_idx + j];

        }
    }

    cv::Mat rvec(rv, true), tvec(tv, true);


   solvePnP(mat3dcontour,
        landmark_points_buffer,
        this->camera_matrix,
        this->camera_distortion,
        rvec,
        tvec,
        !this->complex, //extrinsic guess
        cv::SOLVEPNP_ITERATIVE
    );


    get_euler(rvec, tvec);


    for (int i = 0; i < 3; i++)
    {
        face_data->rotation[i] = rvec.at<double>(i, 0);
        face_data->translation[i] = tvec.at<double>(i, 0) * 10;
    }

    // We dont want the Z axis oversaturated.
    face_data->translation[2] /= 100;

    std::cout << face_data->to_string() << std::endl;

    correct_rotation(*face_data);

}

void PositionSolver::set_prior_pitch(float new_pitch)
{
    this->prior_pitch = new_pitch;
    this->rv[0] = this->prior_pitch;
}

void PositionSolver::set_prior_yaw(float new_yaw)
{
    this->prior_yaw = new_yaw;
    this->rv[1] = this->prior_yaw;
}

void PositionSolver::set_prior_distance(float new_distance)
{
    this->prior_distance = new_distance * -2.;
    this->tv[2] = this->prior_distance;
}


void PositionSolver::get_euler(cv::Mat& rvec, cv::Mat& tvec)
{
    cv::Mat rotM(3, 3, CV_64FC1);
    cv::Rodrigues(rvec, rotM);

    cv::Mat concated(3, 4, CV_64FC1);
    cv::hconcat(rotM, tvec, concated);

    cv::decomposeProjectionMatrix(
        concated,
        cv::Mat(3, 3, CV_64FC1),
        cv::Mat(3, 3, CV_64FC1),
        cv::Mat(4, 1, CV_64FC1),
        cv::noArray(),
        cv::noArray(),
        cv::noArray(),
        rvec
    );

}

void PositionSolver::correct_rotation(FaceData& face_data)
{
    float distance = -(face_data.translation[2]);
    float lateral_offset = face_data.translation[1];
    float verical_offset = face_data.translation[0];

    float correction_yaw = std::atan(std::tan(lateral_offset / distance)) * TO_DEG;
    float correction_pitch = std::atan(std::tan(verical_offset / distance)) * TO_DEG;

    face_data.rotation[1] += correction_yaw;
    face_data.rotation[0] += correction_pitch;
}

