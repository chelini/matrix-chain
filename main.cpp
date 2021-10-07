#include <iostream>
#include <vector>
#include <limits>

using namespace std;

class Matrix {
private:
  vector<int> shape;
  string name;

public:
  vector<int> getShape() { return shape; };
  string getName() { return name; };
  Matrix() = delete;
  Matrix(vector<int> shape) : shape(shape){};
  Matrix(string name, vector<int> shape) : name(name), shape(shape) {};
};

vector<long> getPVector(vector<Matrix> &operands) {
  vector<long> pVector;
  for (Matrix matrix : operands) {
    auto shape = matrix.getShape();
    if (!pVector.size()) {
      pVector.push_back(shape[0]);
      pVector.push_back(shape[1]);
    } else {
      pVector.push_back(shape[1]);
    }
  }
  return pVector;
}

void printOptimalParens(const vector<vector<long>> &s, size_t i,
                        size_t j, vector<Matrix> operands) {
  if (i == j) {
    cout << " ";
    cout << operands[i - 1].getName();
    cout << "  ";
  } else {
    cout << "(";
    printOptimalParens(s, i, s[i][j], operands);
    printOptimalParens(s, s[i][j] + 1, j, operands);
    cout << ")";
  }
}

vector<vector<long>> getOptimalSplit(vector<Matrix> &operands) {

  vector<long> pVector = getPVector(operands);
  const size_t n = pVector.size();
  vector<vector<long>> m(n, vector<long>(n, std::numeric_limits<long>::max()));
  vector<vector<long>> s(n, vector<long>(n, std::numeric_limits<long>::max()));

  for (size_t i = 0; i < n; i++)
    m[i][i] = 0;

  size_t j = 0;
  long q = 0;
  for (size_t l = 2; l < n; l++) {
    for (size_t i = 1; i < n - l + 1; i++) {
      j = i + l - 1;
      m[i][j] = std::numeric_limits<long>::max();
      for (size_t k = i; k <= j - 1; k++) {
        long cost = pVector.at(i - 1) * pVector.at(k) * pVector.at(j);
        q = m[i][k] + m[k + 1][j] + cost;
        if (q < m[i][j]) {
          m[i][j] = q;
          s[i][j] = k;
        }
      }
    }
  }

  cout << "\n\n-----s------\n";
  int rows = s.size();
  int cols = s[0].size();
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      if (s[i][j] == std::numeric_limits<long>::max())
        cout << "- ";
      else
        cout << s[i][j] << " ";
    }
    cout << "\n";
  }
  cout << "-----m------\n";
  rows = m.size();
  cols = m[0].size();
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      if (m[i][j] == std::numeric_limits<long>::max())
        cout << "- ";
      else
        cout << m[i][j] << " ";
    }
    cout << "\n";
  }
  printOptimalParens(s, 1, operands.size(), operands);
  cout << "\n\n";

  return s;
}

int main() { 
  Matrix A = Matrix("A1", {30,35});
  Matrix B = Matrix("A2", {35,15});
  Matrix C = Matrix("A3", {15,5});
  Matrix D = Matrix("A4", {5,10});
  Matrix E = Matrix("A5", {10,20});
  Matrix F = Matrix("A6", {20,25});
  vector<Matrix> matrices = {A, B, C, D, E, F};
  auto split = getOptimalSplit(matrices);
  
  return 0; 

}
