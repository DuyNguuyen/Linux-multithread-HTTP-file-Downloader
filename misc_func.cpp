#include "misc_func.h"

string get_new_file_name(std::string fileName) {
    int count = 1;
    string newFileName = fileName;
    while (true) {
        ifstream file(newFileName);
        if (!file.good()) {
            return newFileName;
        }
        file.close();
        stringstream ss;
        ss << count;
        newFileName = fileName + "_" + ss.str();
        count++;
    }
}

void merge_vector(vector<down_stat>& v, int l, int m, int r, int (*comparator)(const down_stat*, const down_stat*)) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    down_stat L[n1], R[n2];

    for (i = 0; i < n1; i++)
        L[i] = v[l + i];
    for (j = 0; j < n2; j++)
        R[j] = v[m + 1 + j];

    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (comparator(&L[i], &R[j]) <= 0) {
            v[k] = L[i];
            i++;
        } else {
            v[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        v[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        v[k] = R[j];
        j++;
        k++;
    }
}

void merge_sort(vector<down_stat>& v, int l, int r, int (*comparator)(const down_stat*, const down_stat*)) {
    if (l < r) {
        int m = l + (r - l) / 2;
        merge_sort(v, l, m, comparator);
        merge_sort(v, m + 1, r, comparator);
        merge_vector(v, l, m, r, comparator);
    }
}