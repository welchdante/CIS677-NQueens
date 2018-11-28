#include <iostream>
#include <string>
#include <deque>
#include <unordered_set>
#include <vector>
#include <mpi.h>

#define MASTER  0
#define TAG     0

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

int main(int argc, char* argv[])
{
    int my_rank, num_nodes;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);

    if (my_rank != MASTER) {
        if (argc != 2) {
            MPI_Finalize();

            exit(EXIT_SUCCESS);
        }

        int k = stoi(argv[1]);

        int row;

        /* max possible vector size is 2 * k */
        int * recvVector = new int[2 * k];

        int recvSize;
        int sendSolutions = 0;

        MPI_Request req;

        do {

            MPI_Isend(&sendSolutions, 1, MPI_INT, MASTER, TAG, MPI_COMM_WORLD, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);

            sendSolutions = 0;

            MPI_Recv(&recvSize, 1, MPI_INT, MASTER, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (recvSize <= 0) {
                break;
            }

            MPI_Recv(recvVector, recvSize * 2, MPI_INT, MASTER, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            comb_t node;
            last_t next;

            for (int i = 0; i < recvSize; i++) {
                int row = recvVector[i * 2];
                int last = recvVector[i * 2 + 1];
                next = { row, last };
                node.previous.push_back(next);
                node.cols.insert(last);
                node.pos_diags.insert(row + last);
                node.neg_diags.insert(row - last);
            }

            next = { recvSize + 1, 0 };
            node.previous.push_back(next);

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
                        // printSolution(node.previous, i);
                        sendSolutions++;
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
                    node.previous.pop_back();
                    /* check if back to where we started depth-wise */
                    if ((int) node.previous.size() == recvSize) {
                        break;
                    }
                }
            } while (true);

        } while (true);

        delete [] recvVector;
    }
    else {
        if (argc != 2) {
            cout << "Requires one argument:\n\tk -> size of board" << endl;

            MPI_Finalize();

            exit(EXIT_FAILURE);
        }

        int k = stoi(argv[1]);

        int flag;

        int sendSize = -1;

        int sendVectorIndex;
        /* max possible vector size is 2 * k */
        int * sendVector = new int[2 * k];

        int * recvSolutions = new int[num_nodes - 1];
        MPI_Request * requests = new MPI_Request[num_nodes - 1];

        for (int i = 1; i < num_nodes; i++) {
            MPI_Irecv(&recvSolutions[i - 1], 1, MPI_INT, i, TAG, MPI_COMM_WORLD, &requests[i - 1]);
        }

        /* base case */
        if (k == 1) {
            cout << "Solutions:\t1" << endl;

            /* send done signal */
            for (int i = 1; i < num_nodes; i++) {
                MPI_Wait(&requests[i - 1], MPI_STATUS_IGNORE);

                MPI_Send(&sendSize, 1, MPI_INT, i, TAG, MPI_COMM_WORLD);
            }

            delete [] sendVector;
            delete [] recvSolutions;
            delete [] requests;

            MPI_Finalize();

            exit(EXIT_SUCCESS);
        }

        deque<comb_t> combinations;

        int solutions = 0;

        int row = 1;

        /* initialize combinations */
        for (int i = 1; i <= k; i++) {
            comb_t tempComb;

            last_t tempNext = { row, i };
            tempComb.previous.push_back(tempNext);
            tempComb.cols.insert(i);

            tempComb.pos_diags.insert(row + i);
            tempComb.neg_diags.insert(row - i);
            combinations.push_back(tempComb);
        }

        pair<unordered_set<int>::iterator, bool> result;
        while ((int) combinations.size() > 0 && (int) combinations.size() < 10 * (num_nodes - 1)) {
            comb_t curComb = combinations.front();
            combinations.pop_front();
            
            row = curComb.previous.back().row + 1;
            for (int i = 1; i <= k; i++) {

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
                    // printSolution(curComb.previous, i);
                    solutions++;
                    curComb.cols.erase(i);
                    curComb.pos_diags.erase(row + i);
                    curComb.neg_diags.erase(row - i);
                    continue;
                }

                comb_t newComb = curComb;
                last_t curNext = { row, i };
                newComb.previous.push_back(curNext);

                combinations.push_back(newComb);

                curComb.cols.erase(i);
                curComb.pos_diags.erase(row + i);
                curComb.neg_diags.erase(row - i);
            }
        }

        while (combinations.size() > 0) {
            for (int i = 1; i < num_nodes; i++) {
                if (combinations.empty()) {
                    break;
                }
                MPI_Test(&requests[i - 1], &flag, MPI_STATUS_IGNORE);
                if (flag == 0) {
                    continue;
                }
                
                solutions += recvSolutions[i - 1];

                comb_t curComb = combinations.front();
                combinations.pop_front();

                /* send previous (calculate cols, pos_diags, and neg_diags after receiving) */
                sendSize = curComb.previous.size();
                MPI_Send(&sendSize, 1, MPI_INT, i, TAG, MPI_COMM_WORLD);
                
                sendVectorIndex = 0;
                for (auto it = curComb.previous.begin(); it != curComb.previous.end(); it++) {
                    sendVector[sendVectorIndex++] = it->row;
                    sendVector[sendVectorIndex++] = it->last;
                }

                MPI_Send(sendVector, sendVectorIndex, MPI_INT, i, TAG, MPI_COMM_WORLD);

                /* get ready for next time node is ready for work */
                MPI_Irecv(&recvSolutions[i - 1], 1, MPI_INT, i, TAG, MPI_COMM_WORLD, &requests[i - 1]);
            }
        }

        sendSize = -1;
        /* send done signal */
        for (int i = 1; i < num_nodes; i++) {
            MPI_Wait(&requests[i - 1], MPI_STATUS_IGNORE);

            solutions += recvSolutions[i - 1];
            // cout << recvSolutions[i - 1] << " solutions received from node " << i << endl;

            MPI_Send(&sendSize, 1, MPI_INT, i, TAG, MPI_COMM_WORLD);
        }

        delete [] sendVector;
        delete [] recvSolutions;
        delete [] requests;

        cout << "Solutions:\t" << solutions << endl;
    }

    MPI_Finalize();

    return 0;
}
