#include <qt/QtCore/QByteArray>
#include <vector>

using namespace std;

class Frame {
private:

public:
    size_t rows;
    size_t cols;
    size_t components = 3;
    uint8_t* matrixA;

    Frame(int rows, int cols) : rows(rows), cols(cols) {
        matrixA = (uint8_t*)malloc(sizeof(uint8_t) * components * cols * rows);

        this->reset();
    }

    vector<int> to2D(int index) {
        vector<int> xy = {};
        xy.push_back((index / cols ) % rows);
        xy.push_back(index / ( cols * rows ));

        return xy;
    }

    int to1D(int c, int x, int y) {
        int index = x + y * cols + c * cols * rows;

        return index;
    }

    void reset() {
        for(size_t i = 0; i < components * cols * rows; i++) {
            matrixA[i] = 0;
        }
    }

    void set_key(int x, int y, uint8_t red, uint8_t green, uint8_t blue) {
        this->matrixA[to1D(0, x, y)] = red;
        this->matrixA[to1D(1, x, y)] = green;
        this->matrixA[to1D(2, x, y)] = blue;
    }

    vector<uint8_t> get_key(int x, int y) {
        return {this->matrixA[to1D(0, x, y)], this->matrixA[to1D(1, x, y)], this->matrixA[to1D(2, x, y)]};
    }

    QByteArray rowBinary(size_t row) {
        QByteArray byts = QByteArray();

        byts += row;
        byts += (char)0;
        byts += this->cols - 1;

        for(size_t col = 0; col < cols; col++) {
            auto rgb = get_key(col, row);

            byts += rgb[0];
            byts += rgb[1];
            byts += rgb[2];
        }

        return byts;
    }

    QByteArray bytes() {
        QByteArray byteA = QByteArray();

        for(size_t i = 0; i < rows; i++) {
            byteA.append(rowBinary(i));
        }

        return byteA;
    }
};
