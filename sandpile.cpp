#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

struct Args {
    std::string input;
    std::string output;
    int maxIterations = 1000;
    int saveFrequency = 10;
};

struct Sandpile {
    int** data = nullptr;
    int width = 0;
    int height = 0;

    void allocate(int rows, int cols) {
        clear();
        data = new int*[rows];
        for (int i = 0; i < rows; ++i) {
            data[i] = new int[cols]();
        }
        width = cols;
        height = rows;
    }

    void clear() {
        if (data) {
            for (int i = 0; i < height; ++i) {
                delete[] data[i];
            }
            delete[] data;
            data = nullptr;
        }
        width = 0;
        height = 0;
    }

    ~Sandpile() {
        clear();
    }
};

void createDirectory(const std::string& directory) {
    if (CreateDirectory(directory.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
    } else {
        std::cerr << "Error creating directory: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void arguments(int argc, char* argv[], Args& args) {
    std::string token;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            token = argv[i];

            if (token.substr(0, 2) == "--") {
                int poz_arg = token.find("=");
                std::string key = token.substr(0, poz_arg);
                std::string value;

                if (poz_arg == std::string::npos) {
                    value = "";
                } else {
                    value = token.substr(poz_arg + 1);
                }

                if (key == "--output") {
                    args.output = value;
                } else if (key == "--input") {
                    args.input = value;
                } else if (key == "--max") {
                    args.maxIterations = std::stoi(value);
                } else if (key == "--freq") {
                    args.saveFrequency = std::stoi(value);
                }

            } else if (token == "-o") {
                if (i + 1 < argc) {
                    args.output = argv[i++];
                }
            } else if (token == "-i") {
                if (i + 1 < argc) {
                    args.input = argv[i++];
                }
            } else if (token == "-m") {
                if (i + 1 < argc) {
                    args.maxIterations = std::stoi(argv[i++]);
                }
            } else if (token == "-f") {
                if (i + 1 < argc) {
                    args.saveFrequency = std::stoi(argv[i++]);
                }
            }
        }
    }
}

int Min(int a, int b) {
    return (a < b) ? a : b;
}

int Max(int a, int b) {
    return (a > b) ? a : b;
}

void loadInitialState(const std::string& filename, Sandpile& sandpile) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Unable to open input file!" << std::endl;
        exit(EXIT_FAILURE);
    }

    int x, y, grains;
    int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;

    struct Point {
        int x, y, grains;
    };
    Point* points = nullptr;
    int pointCount = 0;

    while (file >> x >> y >> grains) {
        Point* newPoints = new Point[pointCount + 1];
        for (int i = 0; i < pointCount; i++) {
            newPoints[i] = points[i];
        }
        newPoints[pointCount++] = {x, y, grains};
        delete[] points;
        points = newPoints;

        minX = Min(minX, x);
        minY = Min(minY, y);
        maxX = Max(maxX, x);
        maxY = Max(maxY, y);
    }

    sandpile.allocate(maxY - minY + 1, maxX - minX + 1);

    for (int i = 0; i < pointCount; i++) {
        int px = points[i].x - minX;
        int py = points[i].y - minY;
        sandpile.data[py][px] += points[i].grains;
    }

    delete[] points;
}

void expandGrid(Sandpile& sandpile) {
    bool expandTop = false, expandBottom = false, expandLeft = false, expandRight = false;

    for (int x = 0; x < sandpile.width; x++) {
        if (sandpile.data[0][x] > 3) expandTop = true;
        if (sandpile.data[sandpile.height - 1][x] > 3) expandBottom = true;
    }
    for (int y = 0; y < sandpile.height; y++) {
        if (sandpile.data[y][0] > 3) expandLeft = true;
        if (sandpile.data[y][sandpile.width - 1] > 3) expandRight = true;
    }

    if (!expandTop && !expandBottom && !expandLeft && !expandRight) {
        return;
    }

    int newHeight = sandpile.height + (expandTop ? 1 : 0) + (expandBottom ? 1 : 0);
    int newWidth = sandpile.width + (expandLeft ? 1 : 0) + (expandRight ? 1 : 0);

    int** newData = new int*[newHeight];
    for (int i = 0; i < newHeight; i++) {
        newData[i] = new int[newWidth]();
    }

    for (int y = 0; y < sandpile.height; y++) {
        for (int x = 0; x < sandpile.width; x++) {
            newData[y + (expandTop ? 1 : 0)][x + (expandLeft ? 1 : 0)] = sandpile.data[y][x];
        }
    }

    sandpile.clear();

    sandpile.data = newData;
    sandpile.width = newWidth;
    sandpile.height = newHeight;
}


bool isStable(const Sandpile& sandpile) {
    for (int y = 0; y < sandpile.height; y++) {
        for (int x = 0; x < sandpile.width; x++) {
            if (sandpile.data[y][x] > 3) {
                return false;
            }
        }
    }
    return true;
}

void topple(Sandpile& sandpile) {
    Sandpile nextState;
    nextState.allocate(sandpile.height, sandpile.width);

    for (int y = 0; y < sandpile.height; y++) {
        for (int x = 0; x < sandpile.width; x++) {
            int grains = sandpile.data[y][x];
            if (grains > 3) {
                int overflow = grains / 4;
                nextState.data[y][x] += grains % 4;

                if (y > 0) nextState.data[y - 1][x] += overflow;
                if (y < sandpile.height - 1) nextState.data[y + 1][x] += overflow;
                if (x > 0) nextState.data[y][x - 1] += overflow;
                if (x < sandpile.width - 1) nextState.data[y][x + 1] += overflow;
            } else {
                nextState.data[y][x] += grains;
            }
        }
    }

    sandpile.clear();
    sandpile.data = nextState.data;
    sandpile.width = nextState.width;
    sandpile.height = nextState.height;

    nextState.data = nullptr;
}

void saveBMP(const std::string& filename, const Sandpile& sandpile) {
    const int HEADER_SIZE = 54;
    const int PALETTE_SIZE = 64;

    int rowSize = (sandpile.width + 1) / 2;
    int padding = (4 - (rowSize % 4)) % 4;
    int imageSize = (rowSize + padding) * sandpile.height;

    unsigned char header[HEADER_SIZE] = {0};
    header[0] = 'B';
    header[1] = 'M';
    *(int*)&header[2] = HEADER_SIZE + PALETTE_SIZE + imageSize;
    *(int*)&header[10] = HEADER_SIZE + PALETTE_SIZE;
    *(int*)&header[14] = 40;
    *(int*)&header[18] = sandpile.width;
    *(int*)&header[22] = sandpile.height;
    *(short*)&header[26] = 1;
    *(short*)&header[28] = 4;
    *(int*)&header[34] = imageSize;

    unsigned char palette[PALETTE_SIZE] = {
            255, 255, 255, 0,
            120, 200, 80, 0,
            204, 102, 153, 0,
            0, 215, 255, 0,
            0, 0, 0, 0};

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return;
    }

    file.write(reinterpret_cast<const char*>(header), HEADER_SIZE);
    file.write(reinterpret_cast<const char*>(palette), PALETTE_SIZE);

    unsigned char* row = new unsigned char[rowSize + padding]();
    for (int y = sandpile.height - 1; y >= 0; y--) {
        std::fill(row, row + rowSize + padding, 0);
        for (int x = 0; x < sandpile.width; x += 2) {
            unsigned char left = Min(sandpile.data[y][x], 4);
            unsigned char right = (x + 1 < sandpile.width) ? Min(sandpile.data[y][x + 1], 4) : 0;
            row[x / 2] = (left << 4) | right;
        }
        file.write(reinterpret_cast<const char*>(row), rowSize + padding);
    }
    delete[] row;
    file.close();
}

void simulate(Sandpile& sandpile, int maxIterations, int saveFrequency, const std::string& outputDir) {
    for (int iteration = 0; iteration < maxIterations; iteration++) {
        if (iteration % saveFrequency == 0) {
            std::string filename = outputDir + "/sandpile_" + std::to_string(iteration) + ".bmp";
            saveBMP(filename, sandpile);
        }

        if (!isStable(sandpile)) {
            expandGrid(sandpile);
            topple(sandpile);
        } else {
            break;
        }
    }

    std::string finalFilename = outputDir + "/sandpile_final.bmp";
    saveBMP(finalFilename, sandpile);
}

int main(int argc, char* argv[]) {
    Args args;
    arguments(argc, argv, args);
    createDirectory(args.output);

    Sandpile sandpile;
    loadInitialState(args.input, sandpile);

    simulate(sandpile, args.maxIterations, args.saveFrequency, args.output);

    return 0;
}
