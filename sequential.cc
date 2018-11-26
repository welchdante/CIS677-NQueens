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

    deque<comb_t> combinations;

    int solutions = 0;

    /* initialize combinations */
    int row = 1;

    comb_t tempComb;

    last_t next = { row, 1 };
    tempComb.previous.push_back(next);
    tempComb.cols.insert(1);

    next = { 2, 0 };

    tempComb.previous.push_back(next);

    tempComb.pos_diags.insert(row + 1);
    tempComb.neg_diags.insert(row - 1);
    combinations.push_back(tempComb);

    pair<unordered_set<int>::iterator, bool> result;
    int prevCol;
    bool onward = false;
    do {
        comb_t & curComb = combinations.back();
        // combinations.pop_back();
        
        row = curComb.previous.back().row;
        /* remove old placement if applicable */
        prevCol = curComb.previous.back().last;
        if (prevCol > 0) {
            curComb.cols.erase(prevCol);
            curComb.pos_diags.erase(row + prevCol);
            curComb.neg_diags.erase(row - prevCol);
        }
        for (int i = curComb.previous.back().last + 1; i <= k; i++) {

            result = curComb.cols.insert(i);
            if (!result.second) {
                continue;
            }
            result = curComb.pos_diags.insert(row + i);
            if (!result.second) {
                curComb.cols.erase(i);
                continue;
            }
            result = curComb.neg_diags.insert(row - i);
            if (!result.second) {
                curComb.cols.erase(i);
                curComb.pos_diags.erase(row + i);
                continue;
            }
            /* found a valid spot, continue */
            if (row == k) {
                printSolution(curComb.previous, i);
                solutions++;
                curComb.cols.erase(i);
                curComb.pos_diags.erase(row + i);
                curComb.neg_diags.erase(row - i);
                continue;
            }
            curComb.previous.back().last = i;
            next = { row + 1, 0 };

            curComb.previous.push_back(next);
            onward = true;
            break;
        }
        /* done with valid spots */
        if (onward) {
            onward = false;
        } else {
            curComb.previous.pop_back();
        }
    } while (combinations.back().previous.size() > 0);

    cout << "Solutions:\t" << solutions << endl;
}