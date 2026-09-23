

// Custom Matrix class
class Matrix {
private:
    int rows, cols;
    vector<double> data;

public:
    // Constructor
    Matrix(int rows, int cols) : rows(rows), cols(cols), data(rows * cols, 0.0) {}

    // Access element (read/write)
    double& operator()(int i, int j) {
        return data[i * cols + j];
    }

    // Access element (read-only)
    double operator()(int i, int j) const {
        return data[i * cols + j];
    }

    // Set all elements to zero
    void setZero() {
        fill(data.begin(), data.end(), 0.0);
    }

    // Copy data from another matrix
    void copyFrom(const Matrix& other) {
        data = other.data;
    }

    // Get minimum value
    double minCoeff() const {
        return *min_element(data.begin(), data.end());
    }

    // Get maximum value
    double maxCoeff() const {
        return *max_element(data.begin(), data.end());
    }

    // Get number of rows
    int getRows() const { return rows; }

    // Get number of columns
    int getCols() const { return cols; }
};

int main() {
    // Parameters
    double c = 343.0; // Speed of sound in air (m/s)
    double h = 1.0;   // Spatial step size (meters)
    int N = 100;      // Number of spatial grid points along each axis

    // Stability condition (Courant–Friedrichs–Lewy condition)
    double safety_factor = 0.5;
    double delta_t = (h / c) * (1.0 / sqrt(2.0)) * safety_factor;  // Time step size
    double s = c * delta_t / h;  // CFL number
    double s2 = s * s;

    // Initialize wave amplitude matrices
    Matrix u_past(N, N);     // Wave amplitude at time n-1
    Matrix u_present(N, N);  // Wave amplitude at time n
    Matrix u_future(N, N);   // Wave amplitude at time n+1

    // Initial condition: Gaussian pulse at the center
    double xc = (N - 1) * h / 2.0;
    double yc = (N - 1) * h / 2.0;
    double sigma = 5.0 * h;

    for (int i = 0; i < N; ++i) {
        double x = i * h;
        for (int j = 0; j < N; ++j) {
            double y = j * h;
            u_present(i, j) = exp(-((x - xc) * (x - xc) + (y - yc) * (y - yc)) / (2.0 * sigma * sigma));
        }
    }
    u_past.copyFrom(u_present);

    // Set up visualization using OpenCV
    namedWindow("2D Sound Propagation Simulation", WINDOW_AUTOSIZE);

    int frames = 200;
    for (int frame = 0; frame < frames; ++frame) {
        // Compute the wave amplitude at the next time step
        for (int i = 1; i < N - 1; ++i) {
            for (int j = 1; j < N - 1; ++j) {
                u_future(i, j) = 2.0 * u_present(i, j) - u_past(i, j) +
                                 s2 * (u_present(i + 1, j) + u_present(i - 1, j) +
                                       u_present(i, j + 1) + u_present(i, j - 1) -
                                       4.0 * u_present(i, j));
            }
        }
        // Apply boundary conditions (absorbing boundaries)
        for (int i = 0; i < N; ++i) {
            u_future(i, 0) = 0.0;
            u_future(i, N - 1) = 0.0;
            u_future(0, i) = 0.0;
            u_future(N - 1, i) = 0.0;
        }

        // Update the wave amplitude matrices for the next iteration
        u_past.copyFrom(u_present);
        u_present.copyFrom(u_future);

        // Normalize for visualization
        Matrix u_visual = u_present;
        double minVal = u_visual.minCoeff();
        double maxVal = u_visual.maxCoeff();
        double range = maxVal - minVal;
        if (range == 0.0) range = 1.0; // Prevent division by zero
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                u_visual(i, j) = (u_visual(i, j) - minVal) / range;
            }
        }

        // Convert to OpenCV Mat for display
        Mat img(N, N, CV_8UC1);
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                img.at<uchar>(i, j) = static_cast<uchar>(u_visual(i, j) * 255);
            }
        }
        applyColorMap(img, img, COLORMAP_JET);
        imshow("2D Sound Propagation Simulation", img);
        if (waitKey(30) >= 0) break;
    }

    destroyAllWindows();
    return 0;
}
