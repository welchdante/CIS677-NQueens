#include <iostream>
#include <string>
#include <deque>
#include <unordered_set>
#include <vector>

using namespace std;

struct last_t {
    int row;
    int last;
};

struct comb_t {
    vector<last_t> previous;
    unordered_set<int> cols;
    unordered_set<int> pos_diags;
    unordered_set<int> neg_diags;
};

void printSolution(const vector<last_t> & solution, const int & last) {
    cout << "(";
    for (auto it = solution.begin(); it != solution.end() - 1; it++) {
        cout << it->last << ", ";
    }
    cout << last << ")" << endl;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cout << "Requires one argument:\n\tk -> size of board" << endl;
        exit(EXIT_FAILURE);
    }

    int k = stoi(argv[1]);

    /* base case */
    if (k == 1) {
        cout << "Solutions:\t" << 1 << endl;
        exit(EXIT_SUCCESS);
    }

    comb_t node;

    int solutions = 0;

    /* initialize combinations */
    int row = 1;

    last_t next = { row, 1 };
    node.previous.push_back(next);
    node.cols.insert(1);

    next = { 2, 0 };

    node.previous.push_back(next);

    node.pos_diags.insert(row + 1);
    node.neg_diags.insert(row - 1);

    pair<unordered_set<int>::iterator, bool> result;
    int prevCol;
    bool onward = false;
    do {
        
        row = node.previous.back().row;
        /* remove old placement if applicable */
        prevCol = node.previous.back().last;
        if (prevCol > 0) {
            node.cols.erase(prevCol);
            node.pos_diags.erase(row + prevCol);
            node.neg_diags.erase(row - prevCol);
        }
        for (int i = node.previous.back().last + 1; i <= k; i++) {

            result = node.cols.insert(i);
            if (!result.second) {
                continue;
            }
            result = node.pos_diags.insert(row + i);
            if (!result.second) {
                node.cols.erase(i);
                continue;
            }
            result = node.neg_diags.insert(row - i);
            if (!result.second) {
                node.cols.erase(i);
                node.pos_diags.erase(row + i);
                continue;
            }
            /* found a valid spot, continue */
            if (row == k) {
                printSolution(node.previous, i);
                solutions++;
                node.cols.erase(i);
                node.pos_diags.erase(row + i);
                node.neg_diags.erase(row - i);
                continue;
            }
            node.previous.back().last = i;
            next = { row + 1, 0 };

            node.previous.push_back(next);
            onward = true;
            break;
        }
        /* done with valid spots */
        if (onward) {
            onward = false;
        } else {
            break;
        }
    } while (true);

    cout << "Solutions:\t" << solutions << endl;
}